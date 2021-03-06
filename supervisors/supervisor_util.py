import collections
import copy
import errno
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


DEFAULT_PRIORITY = -1


class HazukiJobBase(object):
  def __init__(self, task, priority=DEFAULT_PRIORITY, data=None, cgroup=None):
    self.problem_id = task['id']
    self.seed = task['sourceSeeds'][0]
    self.task = task
    self.priority = priority
    self.data = data
    self.size = task['width'] * task['height']
    self._cgroup = cgroup
    self._proc = None
    self._reader_thread = None
    self.solution = make_sentinel_solution(self.problem_id, self.seed)
    self.returncode = None
    self.start_time = None
    self.end_time = None
    self._finish_callbacks = []

  def register_finish_callback(self, callback):
    self._finish_callbacks.append(callback)

  def set_priority(self, new_priority):
    if not self._proc:
      self.priority = new_priority
      logging.debug('Rescheduled priority: %r', self)

  def start(self):
    logging.debug('Starting: %r', self)
    self._proc = self._make_process()
    if self._cgroup:
      subprocess.call(
        ['sudo', '-n', 'cgclassify', '-g', 'memory:%s' % self._cgroup,
         str(self._proc.pid)])
    self.start_time = time.time()
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
            logging.info('Got score=%d: %r', solution['_score'], self)
    except Exception:
      logging.exception('Uncaught exception in output reader thread: %r', self)

    try:
      self._proc.terminate()
    except Exception:
      pass
    self._proc.wait()
    self._proc.stdout.close()
    self.end_time = time.time()
    logging.debug('Finished in %.3fs: %r', self.end_time - self.start_time, self)
    self._call_callbacks()

  def _call_callbacks(self):
    for callback in self._finish_callbacks:
      try:
        callback(self)
      except Exception:
        logging.exception('Uncaught exception in finish callback: %r', self)


class SolverJob(HazukiJobBase):
  def __init__(self, args, task, priority=DEFAULT_PRIORITY, data=None, cgroup=None):
    super(SolverJob, self).__init__(task=task, priority=priority, data=data, cgroup=cgroup)
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
      #logging.debug('Launching AI: command line: %s', ' '.join(real_args))
      return subprocess.Popen(real_args, stdin=f, stdout=subprocess.PIPE)

  def _signal_thread_main(self):
    try:
      while True:
        time.sleep(1)
        self._proc.send_signal(signal.SIGUSR1)
    except Exception as e:
      if isinstance(e, OSError) and e.errno == errno.ESRCH:
        pass
      else:
        logging.exception('Uncaught exception in signal thread: %r', self)

  def __repr__(self):
    return '<SolverJob p%d/s%d pri=%d %s>' % (
      self.problem_id, self.seed, self.priority, os.path.basename(self.args[0]))


class RewriterJob(HazukiJobBase):
  def __init__(self, args, solution, task, priority=DEFAULT_PRIORITY, data=None, cgroup=None):
    super(RewriterJob, self).__init__(task=task, priority=priority, data=data, cgroup=cgroup)
    self.args = args
    self.original_solution = solution
    self.task = task

  def _make_process(self):
    task_f = tempfile.NamedTemporaryFile()
    json.dump(self.task, task_f)
    task_f.flush()
    task_f.seek(0)
    solution_f = tempfile.NamedTemporaryFile()
    json.dump([self.original_solution], solution_f)
    solution_f.flush()
    solution_f.seek(0)
    real_args = list(self.args) + [
      '--problem=%s' % task_f.name,
      '--output=%s' % solution_f.name,
    ]
    def close_temp_files(self):
      task_f.close()
      solution_f.close()
    self.register_finish_callback(close_temp_files)
    with open(os.devnull, 'w') as devnull:
      return subprocess.Popen(real_args, stdout=subprocess.PIPE)

  def __repr__(self):
    return '<RewriterJob p%d/s%d pri=%d %s>' % (
      self.problem_id, self.seed, self.priority, os.path.basename(self.args[0]))


def run_generic_jobs(jobs, num_threads, soft_deadline, hard_deadline):
  unstarted_jobs = list(jobs)
  started_jobs = []
  finished_jobs = []
  job_order = []
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
        new_job = min(unstarted_jobs, key=lambda job: (job.priority, job.size))
        unstarted_jobs.remove(new_job)
        new_job.register_finish_callback(lambda job: finish_queue.put(job))
        new_job.start()
        started_jobs.append(new_job)
        job_order.append(new_job)

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

  logging.debug('Job execution order:')
  for job in job_order:
    cost = job.end_time - job.start_time if job.end_time else None
    cost_str = '%.3fs%s' % (cost, ' *' if cost >= 1 else '') if cost else 'overrun'
    logging.debug('  %r score=%d time=%s', job, job.solution['_score'], cost_str)


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


def report_to_log_server(solutions, override_tag=None):
  if override_tag:
    solutions = copy.deepcopy(solutions)
    for solution in solutions:
      solution['tag'] = override_tag
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
