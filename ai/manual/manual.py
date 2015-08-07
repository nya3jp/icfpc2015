#!/usr/bin/python

"""AI to submit manual solutions."""

import json
import os
import sys


def parse_arg(argv):
  problems = []
  for i in xrange(1, len(argv), 2):
    if argv[i] == '-f':
      with open(argv[i+1], 'rb') as f:
        problem = json.load(f)
      problems.append(problem)
  return problems


def load_solutions():
  solution_map = {}
  solutions_dir = os.path.join(os.path.dirname(__file__), 'solutions')
  for name in list(os.listdir(solutions_dir)):
    path = os.path.join(solutions_dir, name)
    if path.endswith('.json'):
      with open(path) as f:
        problem_result = json.load(f)
      for seed_result in problem_result:
        key = (seed_result['problemId'], seed_result['seed'])
        if key not in solution_map or seed_result['_score'] > solution_map[key]['_score']:
          solution_map[key] = seed_result
  return solution_map


def main(argv):
  problems = parse_arg(argv)
  solution_map = load_solutions()
  response = []
  for problem in problems:
    problem_id = problem['id']
    for seed in problem['sourceSeeds']:
      solution = solution_map.get((problem_id, seed))
      if solution:
        response.append(solution)
      else:
        response.append({
          'problemId': problem_id,
          'seed': seed,
          'tag': 'nop',
          'solution': '',
          '_score': 0,
        })
  json.dump(response, sys.stdout)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
