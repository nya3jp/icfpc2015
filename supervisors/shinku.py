#!/usr/bin/python

"""Supervisor honoring given constraints.

Usage:
supervisors/shinku.py -f problem.json
"""

import time
g_start_time = time.time()

import logging
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


def run_solvers(tasks, quick_solvers, num_threads, deadline):
  jobs = []
  for quick_solver in quick_solvers:
    for task in tasks:
      job = supervisor_util.SolverJob(
        args=[quick_solver],
        task=task,
        cgroup=CGROUP_NAME)
      jobs.append(job)

  soft_deadline = deadline - 0.5

  start_time = time.time()
  logging.info(
    'Start solver phase: jobs=%d, soft_deadline=%.1fs hard_deadline=%.1fs',
    len(jobs), soft_deadline - start_time, deadline - start_time)

  supervisor_util.run_generic_jobs(jobs, num_threads, soft_deadline, deadline)
  solutions = [job.solution for job in jobs if job.solution['_score'] > 0]

  end_time = time.time()
  logging.info(
    'End solver phase: solutions=%d, time=%.1fs',
    len(solutions), end_time - start_time)

  return solutions


def run_rewriter(solutions, num_threads, deadline):
  # TODO: IMPLMENT THIS!
  now = time.time()
  time_to_deadline = deadline - now
  time.sleep(time_to_deadline)
  return []


def choose_best_solutions(solutions, tasks):
  solution_map = {}
  for task in tasks:
    problem_id = task['id']
    seed = task['sourceSeeds'][0]
    solution_map[(problem_id, seed)] = supervisor_util.make_sentinel_solution(
      problem_id, seed)
  for solution in solutions:
    key = (solution['problemId'], solution['seed'])
    if (key not in solution_map or
        solution['_score'] > solution_map[key]['_score']):
      solution_map[key] = solution
  return solution_map.values()


def solve_tasks(tasks, quick_solvers, num_threads, deadline):
  now = time.time()
  # Give the rewriter 0.5 * #tasks (max 1/5 of allowed time, min 3sec).
  time_to_deadline = deadline - now
  rewrite_time = min(max(3, 0.5 * len(tasks)), time_to_deadline / 5)
  solver_deadline = deadline - rewrite_time

  solutions = run_solvers(tasks, quick_solvers, num_threads, solver_deadline)
  solutions += run_rewriter(solutions, num_threads, deadline)
  return choose_best_solutions(solutions, tasks)


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

  # Impose memory limit with cgroup.
  solver_memlimit = max(1, FLAGS.memlimit - 128)
  cgroup_memlimit_path = (
    '/sys/fs/cgroup/memory/%s/memory.limit_in_bytes' % CGROUP_NAME)
  with open(cgroup_memlimit_path, 'w') as f:
    f.write(str(solver_memlimit * 1024 * 1024))

  solutions = solve_tasks(
    tasks,
    FLAGS.quick_solver,
    num_threads,
    deadline)

  json.dump(solutions, sys.stdout)
  sys.stdout.flush()

  if FLAGS.show_scores:
    supervisor_util.show_scores(solutions)

  if FLAGS.report:
    supervisor_util.report_to_log_server(solutions)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
