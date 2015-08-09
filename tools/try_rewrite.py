#!/usr/bin/python

import os
import subprocess
import sys

import gflags
import requests
import ujson as json

FLAGS = gflags.FLAGS

gflags.DEFINE_string('rewriter', None, '')
gflags.MarkFlagAsRequired('rewriter')


def main(argv):
  for solution_path in argv[1:]:
    with open(solution_path) as f:
      solution = json.load(f)[0]
    problem_path = os.path.join(
      os.path.dirname(__file__), '..', 'problems',
      'problem_%d.json' % solution['problemId'])
    with open(os.devnull, 'w') as devnull:
      output = subprocess.check_output(
        [FLAGS.rewriter, '--problem=%s' % problem_path, '--output=%s' % solution_path],
        stderr=devnull)
    new_solution = json.loads(output)[0]
    if new_solution['_score'] > new_solution['_target_score']:
      print 'OK: %d -> %d (target: %d)' % (solution['_score'], new_solution['_score'], new_solution['_target_score'])
      requests.post(
        'http://solutions.natsubate.nya3.jp/log',
        headers={'Content-Type': 'application/json'},
        data=output)
    else:
      print 'NG: %d -> %d (target: %d)' % (solution['_score'], new_solution['_score'], new_solution['_target_score'])


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
