#!/usr/bin/python

"""Supervisor that runs jobs in parallel.

Usage:
supervisors/parallel.py -f problem.json -c N path/to/solver [extra args to solver...]
"""

import time
g_start_time = time.time()

import logging
import Queue as queue
import sys
import threading

import logging_util
import gflags
import supervisor_util
import ujson as json

FLAGS = gflags.FLAGS

gflags.DEFINE_multistring('problem', None, 'Path to problem JSON.', short_name='f')
gflags.MarkFlagAsRequired('problem')
gflags.DEFINE_multistring('powerphrase', [], 'Power phrase.', short_name='p')
gflags.DEFINE_integer('cores', 0, 'Number of CPU cores.', short_name='c')
gflags.DEFINE_integer('timelimit', 0, 'Time limit in seconds.', short_name='t')
gflags.DEFINE_integer('memlimit', 0, 'Memory limit in megabytes.', short_name='m')

gflags.DEFINE_bool('show_scores', False, 'Show scores.')
gflags.DEFINE_bool('report', True, 'Report the result to log server.')


class JobScheduler(object):
  def __init__(self, tasks, solver_args, num_threads):
    self._num_threads = num_threads
    self._event_queue = queue.Queue()
    self._solution_map = {}
    for task in tasks:
      problem_id = task['id']
      seed = task['sourceSeeds'][0]
      self._solution_map[(problem_id, seed)] = supervisor_util.make_sentinel_solution(
        problem_id, seed)
    self._unstarted_jobs = []
    for task in tasks:
      job = supervisor_util.SolverJob(solver_args, task, self._event_queue)
      self._unstarted_jobs.append(job)
    self._started_jobs = []

  def run(self):
    while self._started_jobs or self._unstarted_jobs:
      # Start jobs as many as possible.
      while self._unstarted_jobs and len(self._started_jobs) < self._num_threads:
        new_job = self._unstarted_jobs.pop(0)
        new_job.start()
        self._started_jobs.append(new_job)

      # Wait for job finish or solution update.
      logging.debug('Waiting for job events...')
      job = self._event_queue.get(timeout=86400)
      logging.debug('Got an event for %r', job)
      if job in self._started_jobs and job.poll() is not None:
        job.wait()
        self._started_jobs.remove(job)
      old_solution = self._solution_map[(job.problem_id, job.seed)]
      new_solution = job.best_solution
      if new_solution['_score'] > old_solution['_score']:
        self._solution_map[(job.problem_id, job.seed)] = new_solution
        logging.info(
          'Updated solution: score=%d by %r', new_solution['_score'], job)
      self._event_queue.task_done()

    return list(self._solution_map.values())


def main(argv):
  logging_util.setup()

  solver_args = argv[1:]

  # Set defaults.
  if FLAGS.cores == 0:
    FLAGS.cores = supervisor_util.get_cores()
  if FLAGS.timelimit:
    logging.warning('Ignoring time limit specified by -t')
  if FLAGS.memlimit:
    logging.warning('Ignoring memory limit specified by -m')

  tasks = supervisor_util.load_tasks(FLAGS.problem)

  logging.info(
    'Input: %d problems, %d tasks',
    len(FLAGS.problem), len(tasks))

  # Use at least two threads for efficiency.
  num_threads = max(2, FLAGS.cores)

  logging.info('Using %d threads', num_threads)

  sched = JobScheduler(tasks, solver_args, num_threads=num_threads)
  final_solutions = sched.run()

  json.dump(final_solutions, sys.stdout)

  if FLAGS.show_scores:
    supervisor_util.show_scores(solutions)

  if FLAGS.report:
    supervisor_util.report_to_log_server(solutions)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
