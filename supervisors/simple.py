#!tools/python.sh

"""Simple Supervisor that just executes given AI without limits.

Usage:
supervisors/simple.py -f problem.json path/to/solver [extra args to solver...]
"""

import copy
import json
import logging
import signal
import subprocess
import sys
import tempfile
import threading

import logging_util
import gflags

FLAGS = gflags.FLAGS

gflags.DEFINE_multistring('problem', None, 'Path to problem JSON.', short_name='f')
gflags.MarkFlagAsRequired('problem')
gflags.DEFINE_multistring('powerphrase', [], 'Power phrase.', short_name='p')
gflags.DEFINE_integer('cores', 0, 'Number of CPU cores.', short_name='c')
gflags.DEFINE_integer('timelimit', 0, 'Time limit in seconds.', short_name='t')
gflags.DEFINE_integer('memlimit', 0, 'Memory limit in megabytes.', short_name='m')


class Solver(object):
  def __init__(self, problem_id, seed, args, proc):
    self._problem_id = problem_id
    self._seed = seed
    self._args = args
    self._proc = proc
    self._reader_thread = threading.Thread(target=self._reader_thread_main)
    self._reader_thread.start()
    self.best_solution = {
      'problemId': problem_id,
      'seed': seed, 
      'tag': 'sentinel',
      'solution': '',
      '_score': 0,
    }

  @classmethod
  def run(cls, args, task):
    logging.info(
      'Running a solver for P%d/S%d: %s',
      task['id'], task['sourceSeeds'][0], ' '.join(args))
    with tempfile.TemporaryFile() as f:
      json.dump(task, f)
      f.flush()
      f.seek(0)
      proc = subprocess.Popen(args, stdin=f, stdout=subprocess.PIPE)
    return cls(task['id'], task['sourceSeeds'][0], args, proc)

  def pause(self):
    logging.info(
      'Pausing a solver for P%d/S%d: %s',
      task['id'], task['sourceSeeds'][0], ' '.join(args))
    try:
      self._proc.send_signal(signal.SIGSTOP)
    except Exception:
      return False
    return True

  def resume(self):
    logging.info(
      'Resuming a solver for P%d/S%d: %s',
      task['id'], task['sourceSeeds'][0], ' '.join(args))
    try:
      self._proc.send_signal(signal.SIGCONT)
    except Exception:
      return False
    return True

  def terminate(self):
    logging.info(
      'Terminating a solver for P%d/S%d: %s',
      task['id'], task['sourceSeeds'][0], ' '.join(args))
    try:
      self._proc.terminate()
    except Exception:
      pass
    self.join()

  def join(self):
    self._proc.wait()
    self._reader_thread.join()

  def _reader_thread_main(self):
    try:
      while True:
        line = self._proc.stdout.readline()
        if not line:
          break
        solutions = json.loads(line)
        for solution in solutions:
          assert solution['problemId'] == self._problem_id
          assert solution['seed'] == self._seed
          assert isinstance(solution['tag'], unicode)
          assert isinstance(solution['solution'], unicode)
          assert isinstance(solution['_score'], int)
          if solution['_score'] > self.best_solution['_score']:
            self.best_solution = solution
    except Exception:
      logging.exception(
        'Uncaught exception in AI output reader thread: P%d/S%d: %s',
        self._problem_id, self._seed, ' '.join(self._args))
    else:
      logging.info(
        'Finished a solver for P%d/S%d: %s',
        self._problem_id, self._seed, ' '.join(self._args))


def main(argv):
  logging_util.setup()

  solver_path = argv[1]
  solver_extra_args = argv[2:]

  solver_args = [solver_path]
  for p in FLAGS.powerphrase:
    solver_args.extend(['-p', p])

  if FLAGS.cores:
    logging.warning('Ignoring CPU cores specified by -c')
  if FLAGS.timelimit:
    logging.warning('Ignoring time limit specified by -t')
  if FLAGS.memlimit:
    logging.warning('Ignoring memory limit specified by -m')

  problems = []
  for path in FLAGS.problem:
    with open(path) as f:
      problems.append(json.load(f))

  tasks = []
  for p in problems:
    for s in p['sourceSeeds']:
      t = copy.copy(p)  # tasks share some elements
      t['sourceSeeds'] = [s]
      tasks.append(t)

  solutions = []
  for task in tasks:
    solver = Solver.run(solver_args, task)
    solver.join()
    solutions.append(solver.best_solution)

  json.dump(solutions, sys.stdout)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
