#!/usr/bin/python

"""AI to answer known solutions."""

import os
import sys

import ujson as json


def main(unused_argv):
  task = json.load(sys.stdin)
  json_path = os.path.join(
    os.path.dirname(__file__), '..', '..', 'solutions',
    'state-of-the-art.json')
  with open(json_path) as f:
    solutions = json.load(f)
  for solution in solutions:
    if (solution['problemId'] == task['id'] and
        solution['seed'] == task['sourceSeeds'][0]):
      json.dump([solution], sys.stdout)
      sys.stdout.write('\n')


if __name__ == '__main__':
  sys.exit(main(sys.argv))
