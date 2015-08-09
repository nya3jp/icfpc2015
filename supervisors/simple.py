#!/usr/bin/python

"""Simple Supervisor that just executes given AI without limits.

Usage:
supervisors/simple.py -f problem.json path/to/solver [extra args to solver...]
"""

import logging
import sys

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


def main(argv):
  logging_util.setup()

  solver_path = argv[1]
  solver_extra_args = argv[2:]

  solver_args = [solver_path]
  for p in FLAGS.powerphrase:
    solver_args.extend(['-p', p])
  solver_args.extend(solver_extra_args)

  if FLAGS.cores:
    logging.warning('Ignoring CPU cores specified by -c')
  if FLAGS.timelimit:
    logging.warning('Ignoring time limit specified by -t')
  if FLAGS.memlimit:
    logging.warning('Ignoring memory limit specified by -m')

  tasks = supervisor_util.load_tasks(FLAGS.problem)

  solutions = []
  for task in tasks:
    job = supervisor_util.SolverJob(solver_args, task)
    job.start()
    job.wait()
    solutions.append(job.solution)

  json.dump(solutions, sys.stdout)

  if FLAGS.show_scores:
    supervisor_util.show_scores(solutions)

  if FLAGS.report:
    supervisor_util.report_to_log_server(solutions)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
