#!/usr/bin/python

"""Supervisor honoring given constraints.

Usage:
supervisors/shinku.py -f problem.json
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

gflags.DEFINE_multistring('quick_solver', [], 'Path to quick solver.')
gflags.DEFINE_bool('show_scores', False, 'Show scores.')
gflags.DEFINE_bool('report', True, 'Report the result to log server.')

CGROUP_NAME = 'natsubate'


class JobScheduler(object):
  def __init__(self, tasks, quick_solvers, num_threads, deadline):
    self._tasks = tasks
    self._num_threads = num_threads
    self._deadline = deadline
    self._event_queue = queue.Queue()
    self._solution_map = {}
    for task in tasks:
      problem_id = task['id']
      seed = task['sourceSeeds'][0]
      self._solution_map[(problem_id, seed)] = supervisor_util.make_sentinel_solution(
        problem_id, seed)
    self._unstarted_jobs = []
    for quick_solver in quick_solvers:
      for task in tasks:
        job = supervisor_util.SolverJob(
          [quick_solver], task, self._event_queue, cgroup=CGROUP_NAME)
        self._unstarted_jobs.append(job)
    self._started_jobs = []

  def run(self):
    solutions = self._run_solvers()
    solutions = self._run_rewriter(solutions)
    return solutions

  def _run_solvers(self):
    logging.debug('JobScheduler: solver phase')

    now = time.time()
    # Give the rewriter 0.5 * #tasks (max 1/5 of allowed time, min 3sec).
    time_to_deadline = self._deadline - now
    rewrite_time = min(max(3, 0.5 * len(self._tasks)), time_to_deadline / 5)
    interrupt_grace_time = 1
    hard_deadline = self._deadline - rewrite_time
    soft_deadline = hard_deadline - interrupt_grace_time
    logging.info(
      'Solver deadline info: rewrite_time=%f, interrupt_grace_time=%f, '
      'soft_deadline=%f, hard_deadline=%f',
      rewrite_time, interrupt_grace_time,
      soft_deadline - now, hard_deadline - now)

    interrupted = False

    while self._started_jobs or self._unstarted_jobs:
      now = time.time()
      if now >= hard_deadline:
        break
      if now >= soft_deadline and not interrupted:
        for job in self._started_jobs:
          job.interrupt()
        interrupted = True

      # Start jobs as many as possible.
      now = time.time()
      if now < soft_deadline:
        while self._unstarted_jobs and len(self._started_jobs) < self._num_threads:
          new_job = self._unstarted_jobs.pop(0)
          new_job.start()
          self._started_jobs.append(new_job)

      # Wait for job finish or solution update.
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
        job = self._event_queue.get(timeout=timeout)
      except queue.Empty:
        if timeout_hard:
          break
        continue

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

    # Terminate orphan jobs.
    for job in self._started_jobs:
      job.terminate()

    return list(self._solution_map.values())

  def _run_rewriter(self, solutions):
    logging.debug('JobScheduler: rewriter phase')

    # TODO: IMPLMENT THIS!
    now = time.time()
    time_to_deadline = self._deadline - now
    time.sleep(time_to_deadline)
    return solutions


def main(unused_argv):
  logging_util.setup()

  # Set defaults.
  if FLAGS.cores == 0:
    FLAGS.cores = supervisor_util.get_cores()
  if FLAGS.timelimit == 0:
    FLAGS.timelimit = 24 * 60 * 60
  if FLAGS.memlimit == 0:
    FLAGS.memlimit = supervisor_util.get_free_memory() - 64

  logging.info(
    'Limits: %d cores, %d seconds, %d megabytes',
    FLAGS.cores, FLAGS.timelimit, FLAGS.memlimit)

  tasks = supervisor_util.load_tasks(FLAGS.problem)

  logging.info(
    'Input: %d problems, %d tasks',
    len(FLAGS.problem), len(tasks))

  # Use at least two threads for efficiency.
  num_threads = max(2, FLAGS.cores)

  deadline = g_start_time + FLAGS.timelimit - 1

  solver_memlimit = max(1, FLAGS.memlimit - 128)
  cgroup_memlimit_path = (
    '/sys/fs/cgroup/memory/%s/memory.limit_in_bytes' % CGROUP_NAME)
  with open(cgroup_memlimit_path, 'w') as f:
    f.write(str(solver_memlimit * 1024 * 1024))

  sched = JobScheduler(
    tasks=tasks,
    quick_solvers=FLAGS.quick_solver,
    num_threads=num_threads,
    deadline=deadline)
  solutions = sched.run()

  json.dump(solutions, sys.stdout)
  sys.stdout.flush()

  if FLAGS.show_scores:
    supervisor_util.show_scores(solutions)

  if FLAGS.report:
    supervisor_util.report_to_log_server(solutions)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
