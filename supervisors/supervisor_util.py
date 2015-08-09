import collections
import copy
import logging
import os
import Queue as queue
import signal
import subprocess
import tempfile
import time
import threading

import gflags
import ujson as json

FLAGS = gflags.FLAGS

gflags.DEFINE_bool('enable_hazuki_proxy', False, '')


class HazukiJobBase(object):
  def __init__(self, problem_id, seed, cgroup=None):
    self.problem_id = problem_id
    self.seed = seed
    self._cgroup = cgroup
    self._proc = None
    self._reader_thread = None
    self.solution = make_sentinel_solution(self.problem_id, self.seed)
    self.returncode = None
    self._start_time = None
    self._end_time = None
    self._finish_callbacks = []

  def register_finish_callback(self, callback):
    self._finish_callbacks.append(callback)

  def start(self):
    logging.debug('Starting: %r', self)
    self._proc = self._make_process()
    if self._cgroup:
      subprocess.call(
        ['sudo', 'cgclassify', '-g', 'memory:%s' % self._cgroup, str(self._proc.pid)])
    self._start_time = time.time()
    self._reader_thread = threading.Thread(target=self._reader_thread_main)
    self._reader_thread.daemon = True
    self._reader_thread.start()

  def _make_process(self):
    raise NotImplementedError()

  def interrupt(self):
    if not self._proc:
      logging.error('Attempted to interrupt an unstarted job: %r', self)
      return
    logging.debug('Interrupting: %r', self)
    try:
      self._proc.send_signal(signal.SIGINT)
    except Exception:
      pass

  def terminate(self):
    if not self._proc:
      logging.error('Attempted to terminate an unstarted job: %r', self)
      return
    logging.debug('Terminating: %r', self)
    try:
      self._proc.send_signal(signal.SIGKILL)
    except Exception:
      pass

  def poll(self):
    returncode = self._proc.poll()
    if returncode is not None and self.returncode is None:
      self.returncode = returncode
      self._reader_thread.join()
      if returncode:
        logging.warning(
          'A job abnormally finished with return code %d: %r', returncode, self)
    return returncode

  def wait(self):
    returncode = self._proc.wait()
    if self.returncode is None:
      self.returncode = returncode
      self._reader_thread.join()
      if returncode:
        logging.warning(
          'A job abnormally finished with return code %d: %r', returncode, self)
    return returncode

  def _reader_thread_main(self):
    try:
      while True:
        line = self._proc.stdout.readline()
        if not line:
          break
        if not line.strip():
          continue
        solutions = json.loads(line)
        for solution in solutions:
          assert isinstance(solution['tag'], unicode)
          assert isinstance(solution['solution'], unicode)
          assert isinstance(solution['_score'], int)
          if solution['_score'] > self.solution['_score']:
            solution['problemId'] = self.problem_id
            solution['seed'] = self.seed
            self.solution = solution
    except Exception:
      logging.exception('Uncaught exception in output reader thread: %r', self)
    else:
      logging.debug('Finished: %r', self)

    try:
      self._proc.terminate()
    except Exception:
      pass
    self._proc.wait()
    self._end_time = time.time()
    logging.debug('Elapsed %.3fs: %r', self._end_time - self._start_time, self)
    self._call_callbacks()

  def _call_callbacks(self):
    for callback in self._finish_callbacks:
      try:
        callback(self)
      except Exception:
        logging.exception('Uncaught exception in finish callback: %r', self)


class SolverJob(HazukiJobBase):
  def __init__(self, args, task, cgroup=None):
    super(SolverJob, self).__init__(
      problem_id=task['id'], seed=task['sourceSeeds'][0], cgroup=cgroup)
    self.args = args
    self.task = task
    self._signal_thread = None

  def start(self):
    super(SolverJob, self).start()
    self._signal_thread = threading.Thread(target=self._signal_thread_main)
    self._signal_thread.daemon = True
    self._signal_thread.start()

  def _make_process(self):
    real_args = list(self.args)
    if FLAGS.enable_hazuki_proxy:
      real_args = [os.path.join(os.path.dirname(__file__), 'hazuki_proxy')] + real_args
    with tempfile.TemporaryFile() as f:
      json.dump(self.task, f)
      f.flush()
      f.seek(0)
      logging.debug('Launching AI: command line: %s', ' '.join(real_args))
      return subprocess.Popen(real_args, stdin=f, stdout=subprocess.PIPE)

  def _signal_thread_main(self):
    try:
      while True:
        time.sleep(1)
        logging.debug('Sending SIGUSR1: %r', self)
        self._proc.send_signal(signal.SIGUSR1)
    except Exception as e:
      logging.debug('Signal thread stopped: %r: %r', self, e)

  def __repr__(self):
    return '<SolverJob p%d/s%d %s>' % (
      self.problem_id, self.seed, os.path.basename(self.args[0]))


def run_generic_jobs(jobs, num_threads, soft_deadline, hard_deadline):
  unstarted_jobs = list(jobs)
  started_jobs = []
  finished_jobs = []
  finish_queue = queue.Queue()
  soft_deadline_triggered = False

  while started_jobs or (not soft_deadline_triggered and unstarted_jobs):
    # Interrupt jobs when soft deadline is passed.
    now = time.time()
    if now >= hard_deadline:
      break
    if now >= soft_deadline and not soft_deadline_triggered:
      for job in started_jobs:
        job.interrupt()
      soft_deadline_triggered = True

    # Start jobs as many as possible.
    if not soft_deadline_triggered:
      while unstarted_jobs and len(started_jobs) < num_threads:
        new_job = unstarted_jobs.pop(0)
        new_job.register_finish_callback(lambda job: finish_queue.put(job))
        new_job.start()
        started_jobs.append(new_job)

    # Wait for job finish.
    now = time.time()
    if now < soft_deadline:
      timeout = soft_deadline - now
      timeout_hard = False
    elif now < hard_deadline:
      timeout = hard_deadline - now
      timeout_hard = True
    else:
      break
    try:
      finishing_job = finish_queue.get(timeout=timeout)
    except queue.Empty:
      if timeout_hard:
        break
      continue
    finishing_job.wait()
    started_jobs.remove(finishing_job)
    finished_jobs.append(finishing_job)
    finish_queue.task_done()

  # Terminate orphan jobs.
  for job in started_jobs:
    job.terminate()

  logging.info(
    'Job stats: %d finished, %d terminated, %d unstarted',
    len(finished_jobs), len(started_jobs), len(unstarted_jobs))


def make_sentinel_solution(problem_id, seed):
  return {
    'problemId': problem_id,
    'seed': seed, 
    'tag': 'sentinel',
    'solution': '',
    '_score': 0,
  }


def load_tasks(problem_paths):
  problems = []
  for path in problem_paths:
    with open(path) as f:
      problems.append(json.load(f))

  tasks = []
  for p in problems:
    for s in p['sourceSeeds']:
      t = copy.copy(p)  # tasks share some elements
      t['sourceSeeds'] = [s]
      tasks.append(t)

  return tasks


def get_cores():
  return int(subprocess.check_output(['nproc']).strip())


def get_free_memory():
  output = subprocess.check_output(['free', '-m'])
  free_mem = int(output.splitlines()[2].split()[-1])
  return free_mem


def show_scores(solutions):
  sorted_solutions = sorted(
    solutions, key=lambda solution: (solution['problemId'], solution['seed']))
  logging.info('Score stats:')
  for solution in sorted_solutions:
    logging.info(
      'p%d/s%d score=%d tag=%s',
      solution['problemId'], solution['seed'], solution['_score'], solution['tag'])
  scores_by_problem_id = collections.defaultdict(list)
  for solution in sorted_solutions:
    scores_by_problem_id[solution['problemId']].append(solution['_score'])
  for problem_id, scores in sorted(scores_by_problem_id.items()):
    logging.info('p%d/avg score=%d', problem_id, sum(scores) / len(scores))


def report_to_log_server(solutions):
  import requests  # Defer loading the module to speed up the boot in prod
  try:
    requests.post(
      'http://solutions.natsubate.nya3.jp/log',
      headers={'Content-Type': 'application/json'},
      data=json.dumps(solutions))
  except Exception:
    logging.exception('An exception raised while reporting solutions to log server.')
  else:
    logging.info('Reported solutions to the log server.')
