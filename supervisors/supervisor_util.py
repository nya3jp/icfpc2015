import copy
import json
import logging
import os
import signal
import subprocess
import tempfile
import time
import threading


class SolverJob(object):
  def __init__(self, args, task, event_queue=None):
    self.args = args
    self.task = task
    self.event_queue = event_queue
    self.problem_id = task['id']
    self.seed = task['sourceSeeds'][0]
    self._proc = None
    self._reader_thread = None
    self.best_solution = make_sentinel_solution(self.problem_id, self.seed)
    self.returncode = None
    self._start_time = None
    self._end_time = None

  def start(self):
    logging.debug('Starting: %r', self)
    with tempfile.TemporaryFile() as f:
      json.dump(self.task, f)
      f.flush()
      f.seek(0)
      self._proc = subprocess.Popen(self.args, stdin=f, stdout=subprocess.PIPE)
    self._start_time = time.time()
    self._reader_thread = threading.Thread(target=self._reader_thread_main)
    self._reader_thread.start()

  def pause(self):
    if not self._proc:
      logging.error('Attempted to pause an unstarted job: %r', self)
      return False
    logging.debug('Pausing: %r', self)
    try:
      self._proc.send_signal(signal.SIGSTOP)
    except Exception:
      return False
    return True

  def resume(self):
    if not self._proc:
      logging.error('Attempted to resume an unstarted job: %r', self)
      return False
    logging.debug('Resuming: %r', self)
    try:
      self._proc.send_signal(signal.SIGCONT)
    except Exception:
      return False
    return True

  def terminate(self):
    if not self._proc:
      logging.error('Attempted to terminate an unstarted job: %r', self)
      return
    logging.debug('Terminating: %r', self)
    try:
      self._proc.terminate()
    except Exception:
      pass
    self.wait()

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
        solutions = json.loads(line)
        for solution in solutions:
          assert solution['problemId'] == self.problem_id
          assert solution['seed'] == self.seed
          assert isinstance(solution['tag'], unicode)
          assert isinstance(solution['solution'], unicode)
          assert isinstance(solution['_score'], int)
          if solution['_score'] > self.best_solution['_score']:
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
