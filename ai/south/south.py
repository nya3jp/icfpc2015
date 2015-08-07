#!/usr/bin/python

"""Mock AI to do nothing and get 0 point for all problems."""

import json
import sys


def parse_arg(argv):
  problems = []
  for i in xrange(1, len(argv), 2):
    if argv[i] == '-f':
      with open(argv[i+1], 'rb') as f:
        problem = json.load(f)
      problems.append(problem)
  return problems


def main(argv):
  problems = parse_arg(argv)
  response = []
  for problem in problems:
    for seed in problem['sourceSeeds']:
      response.append({
        'problemId': problem['id'],
        'seed': seed,
        'tag': 'nop',
        'solution': 'ao',
        '_score': 0,
      })
  json.dump(response, sys.stdout)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
