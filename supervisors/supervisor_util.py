import collections
import copy
import logging
import os
import signal
import subprocess
import tempfile
import time
import threading

import gflags
import ujson as json

FLAGS = gflags.FLAGS

gflags.DEFINE_bool('enable_hazuki_proxy', False, '')


class SolverJob(object):
  def __init__(self, args, task, event_queue=None, cgroup=None):
    self.args = args
    self.task = task
    self.event_queue = event_queue
    self._cgroup = cgroup
    self.problem_id = task['id']
    self.seed = task['sourceSeeds'][0]
    self._proc = None
    self._reader_thread = None
    self._signal_thread = None
    self.best_solution = make_sentinel_solution(self.problem_id, self.seed)
    self.returncode = None
    self._start_time = None
    self._end_time = None

  def start(self):
    logging.debug('Starting: %r', self)
    real_args = list(self.args)
    if FLAGS.enable_hazuki_proxy:
      real_args = [os.path.join(os.path.dirname(__file__), 'hazuki_proxy')] + real_args
    with tempfile.TemporaryFile() as f:
      json.dump(self.task, f)
      f.flush()
      f.seek(0)
      logging.debug('Launching AI: command line: %s', ' '.join(real_args))
      self._proc = subprocess.Popen(real_args, stdin=f, stdout=subprocess.PIPE)
    if self._cgroup:
      subprocess.call(
        ['sudo', 'cgclassify', '-g', 'memory:%s' % self._cgroup, str(self._proc.pid)])
    self._start_time = time.time()
    self._reader_thread = threading.Thread(target=self._reader_thread_main)
    self._reader_thread.daemon = True
    self._reader_thread.start()
    self._signal_thread = threading.Thread(target=self._signal_thread_main)
    self._signal_thread.daemon = True
    self._signal_thread.start()

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
          if solution['_score'] > self.best_solution['_score']:
            solution['problemId'] = self.problem_id
            solution['seed'] = self.seed
            self.best_solution = solution
            self._maybe_notify()
    except Exception:
      logging.exception('Uncaught exception in output reader thread: %r', self)
    else:
      logging.debug('Finished: %r', self)
    try:
      self._proc.terminate()
    except Exception:
      pass
    # Make sure the process is terminated before sending notification.
    self._proc.wait()
    self._end_time = time.time()
    logging.debug('Elapsed %.3fs: %r', self._end_time - self._start_time, self)
    self._maybe_notify()

  def _signal_thread_main(self):
    try:
      while True:
        time.sleep(1)
        logging.debug('Sending SIGUSR1: %r', self)
        self._proc.send_signal(signal.SIGUSR1)
    except Exception as e:
      logging.debug('Signal thread stopped: %r: %r', self, e)

  def _maybe_notify(self):
    if self.event_queue:
      self.event_queue.put(self)

  def __repr__(self):
    return '<SolverJob p%d/s%d %s>' % (
      self.problem_id, self.seed, os.path.basename(self.args[0]))



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
