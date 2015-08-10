#!/usr/bin/python

"""Supervisor honoring given constraints.

Usage:
supervisors/shinku.py -f problem.json
"""

import time
g_start_time = time.time()

import collections
import copy
import logging
import os
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

gflags.DEFINE_bool('disable_cgroup', False, 'Disable cgroup.')
gflags.DEFINE_bool('use_state_of_the_art', False, 'Use state-of-the-art.json.')
gflags.DEFINE_multistring('quick_solver', [], 'Path to quick solver.')
gflags.DEFINE_multistring('heavy_solver', [], 'Path to heavy solver.')
gflags.DEFINE_multistring('extra_solver', [], 'Path to extra solver.')
gflags.DEFINE_string('rewriter', None, 'Path to rewriter.')
gflags.DEFINE_bool('show_scores', False, 'Show scores.')
gflags.DEFINE_bool('report', True, 'Report the result to log server.')
gflags.DEFINE_string('report_tag', None, 'Overrides tag on reporting.')
gflags.DEFINE_bool('strip_extra_fields', False, 'Strips non-official fields.')

CGROUP_NAME = 'natsubate'


def load_state_of_the_art(tasks):
  if not FLAGS.use_state_of_the_art:
    return []

  json_path = os.path.join(
    os.path.dirname(__file__), '..', 'solutions', 'state-of-the-art.json')
  with open(json_path) as f:
    solutions = json.load(f)

  known_problem_seeds = set((task['id'], task['sourceSeeds'][0]) for task in tasks)
  solutions = [
    solution for solution in solutions
    if (solution['problemId'], solution['seed']) in known_problem_seeds]

  for solution in solutions:
    solution['tag'] = 'state-of-the-art'

  return solutions


def register_reschedule_callbacks(secondary_job, primary_jobs):
  finished_primary_jobs = set()

  def finish_callback(finishing_job):
    finished_primary_jobs.add(finishing_job)
    if len(finished_primary_jobs) != len(primary_jobs):
      return
    quick_jobs = [job for job in primary_jobs if job.data == 'quick']
    heavy_jobs = [job for job in primary_jobs if job.data != 'quick']
    quick_jobs.sort(key=lambda job: job.solution['_score'], reverse=True)
    heavy_jobs.sort(key=lambda job: job.solution['_score'], reverse=True)
    quick_score = quick_jobs[0].solution['_score']
    heavy_score = heavy_jobs[0].solution['_score']
    if heavy_score <= quick_score:
      base_priority = 700
    else:
      base_priority = 300
    for i, heavy_job in enumerate(heavy_jobs):
      if secondary_job.args == heavy_job.args:
        secondary_job.set_priority(base_priority + i)
        break
    else:
        secondary_job.set_priority(283283283)

  for job in primary_jobs:
    job.register_finish_callback(finish_callback)


def run_solvers(tasks, num_threads, deadline):
  jobs = []

  primary_tasks = []
  secondary_tasks = []
  primary_task_map = {}
  for task in tasks:
    if task['id'] not in primary_task_map:
      primary_task_map[task['id']] = task
      primary_tasks.append(task)
    else:
      secondary_tasks.append(task)

  primary_jobs_map = collections.defaultdict(list)
  for quick_solver in FLAGS.quick_solver:
    for task in tasks:
      job = supervisor_util.SolverJob(
        args=[quick_solver],
        task=task,
        priority=100,
        data='quick',
        cgroup=None if FLAGS.disable_cgroup else CGROUP_NAME)
      jobs.append(job)
      if task is primary_task_map[job.problem_id]:
        primary_jobs_map[job.problem_id].append(job)

  for heavy_solver in FLAGS.heavy_solver:
    for task in primary_tasks:
      job = supervisor_util.SolverJob(
        args=[heavy_solver],
        task=task,
        priority=200,
        data='heavy',
        cgroup=None if FLAGS.disable_cgroup else CGROUP_NAME)
      jobs.append(job)
      primary_jobs_map[job.problem_id].append(job)

  for heavy_solver in FLAGS.heavy_solver:
    for task in secondary_tasks:
      job = supervisor_util.SolverJob(
        args=[heavy_solver],
        task=task,
        priority=500,
        data='heavy',
        cgroup=None if FLAGS.disable_cgroup else CGROUP_NAME)
      jobs.append(job)
      register_reschedule_callbacks(job, primary_jobs_map[job.problem_id])

  for extra_solver in FLAGS.extra_solver:
    for task in tasks:
      job = supervisor_util.SolverJob(
        args=[extra_solver],
        task=task,
        priority=900,
        data='extra',
        cgroup=None if FLAGS.disable_cgroup else CGROUP_NAME)
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


def run_rewriter(original_solutions, tasks, num_threads, deadline):
  if not FLAGS.rewriter:
    logging.warning('Rewriter not available. Scores will suffer.')
    return []

  rewriter_args = [FLAGS.rewriter]
  rewriter_args.extend(['-p', ','.join(FLAGS.powerphrase)])

  task_map = {}
  for task in tasks:
    task_map[(task['id'], task['sourceSeeds'][0])] = task

  original_solutions = sorted(
    original_solutions, key=lambda solution: solution['_score'], reverse=True)
  solution_rank_map = collections.defaultdict(lambda: 0)

  jobs = []
  for solution in original_solutions:
    key = (solution['problemId'], solution['seed'])
    priority = solution_rank_map[key]
    solution_rank_map[key] += 1
    job = supervisor_util.RewriterJob(
      args=rewriter_args,
      solution=solution,
      priority=priority,
      task=task_map[(solution['problemId'], solution['seed'])],
      cgroup=None if FLAGS.disable_cgroup else CGROUP_NAME)
    jobs.append(job)

  soft_deadline = deadline - 0.5

  start_time = time.time()
  logging.info(
    'Start rewriter phase: jobs=%d, soft_deadline=%.1fs hard_deadline=%.1fs',
    len(jobs), soft_deadline - start_time, deadline - start_time)

  supervisor_util.run_generic_jobs(jobs, num_threads, soft_deadline, deadline)
  solutions = [job.solution for job in jobs if job.solution['_score'] > 0]

  end_time = time.time()
  logging.info(
    'End rewriter phase: solutions=%d, time=%.1fs',
    len(solutions), end_time - start_time)

  return solutions


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


def solve_tasks(tasks, num_threads, deadline):
  task_size_total = sum(task['width'] * task['height'] for task in tasks)

  now = time.time()
  time_to_deadline = deadline - now
  rewrite_time = min(max(3, 0.001 * task_size_total), time_to_deadline / 5)
  solver_deadline = deadline - rewrite_time

  known_solutions = load_state_of_the_art(tasks)
  plain_solutions = run_solvers(tasks, num_threads, solver_deadline)
  rewritten_solutions = run_rewriter(plain_solutions, tasks, num_threads, deadline)
  return choose_best_solutions(
    known_solutions + plain_solutions + rewritten_solutions, tasks)


def main(unused_argv):
  logging_util.setup()

  # Set defaults.
  if FLAGS.cores == 0:
    FLAGS.cores = supervisor_util.get_cores()
  if FLAGS.timelimit == 0:
    FLAGS.timelimit = 24 * 60 * 60
  if FLAGS.memlimit == 0:
    FLAGS.memlimit = supervisor_util.get_free_memory() - 64
  if not FLAGS.powerphrase:
    with open(os.path.join(os.path.dirname(__file__), '..', 'power_phrases.txt')) as f:
      FLAGS.powerphrase = f.read().splitlines()

  logging.info(
    'Limits: %d cores, %d seconds, %d megabytes',
    FLAGS.cores, FLAGS.timelimit, FLAGS.memlimit)

  tasks = supervisor_util.load_tasks(FLAGS.problem)

  logging.info(
    'Input: %d problems, %d tasks',
    len(FLAGS.problem), len(tasks))

  num_threads = FLAGS.cores * 2

  deadline = g_start_time + FLAGS.timelimit - 1

  # Impose memory limit with cgroup.
  if not FLAGS.disable_cgroup:
    solver_memlimit = max(1, FLAGS.memlimit - 128)
    cgroup_memlimit_path = (
      '/sys/fs/cgroup/memory/%s/memory.limit_in_bytes' % CGROUP_NAME)
    try:
      with open(cgroup_memlimit_path, 'w') as f:
        f.write(str(solver_memlimit * 1024 * 1024))
    except Exception:
      logging.exception(
        'Failed to set cgroup limit. Maybe you have not run "make"? '
        'If you want to run without cgroup, specify --disable_cgroup.')

  solutions = solve_tasks(tasks, num_threads, deadline)

  output_solutions = solutions
  if FLAGS.strip_extra_fields:
    output_solutions = copy.deepcopy(output_solutions)
    for solution in output_solutions:
      for key in solution.keys():
        if key.startswith('_'):
          del solution[key]

  json.dump(output_solutions, sys.stdout)
  sys.stdout.flush()

  if FLAGS.show_scores:
    supervisor_util.show_scores(solutions)

  if FLAGS.report:
    supervisor_util.report_to_log_server(solutions, override_tag=FLAGS.report_tag)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
