#!/usr/bin/python

"""Master AI."""

import json
import os
import subprocess
import sys

SLAVE_AI_NAME = 'nop'


def parse_args(argv):
  problem_paths = []
  extra_args = []
  for i in xrange(1, len(argv), 2):
    if argv[i] == '-f':
      problem_paths.append(argv[i+1])
    else:
      extra_args.extend(argv[i : (i+2)])
  return problem_paths, extra_args


def load_problem(path):
  with open(path) as f:
    return json.load(f)


def is_known_problem(problem_id):
  path = os.path.join(
    os.path.dirname(__file__), '..', '..', 'problems',
    'problem_%d.json' % problem_id)
  return os.path.exists(path)


def compute_problems(unknown_problem_paths, extra_args):
  cwd = os.path.join(os.path.dirname(__file__), '..', SLAVE_AI_NAME)
  args = ['./play_icfp2015']
  for path in unknown_problem_paths:
    args.extend(['-f', path])
  args.extend(extra_args)
  output = subprocess.check_output(args, cwd=cwd)
  return json.loads(output)


def load_known_solutions():
  with open(os.path.join(
      os.path.dirname(__file__), '..', '..', 'solutions',
      'state-of-the-art.json')) as f:
    return json.load(f)


def fixup_solution(solution):
  solution = solution.copy()
  for key in solution.keys():
    if key.startswith('_'):
      del solution[key]
  return solution


def main(argv):
  input_problem_paths, extra_args = parse_args(argv)
  input_problems = [load_problem(path) for path in input_problem_paths]

  unknown_problem_paths = []
  for problem_path, problem in zip(input_problem_paths, input_problems):
    if not is_known_problem(problem['id']):
      unknown_problem_paths.append(problem_path)

  all_solutions = compute_problems(unknown_problem_paths, extra_args)

  all_solutions.extend(load_known_solutions())

  all_solution_map = {}
  for solution in all_solutions:
    all_solution_map[(solution['problemId'], solution['seed'])] = solution

  final_solutions = []
  for problem in input_problems:
    for seed in problem['sourceSeeds']:
      final_solutions.append(
        fixup_solution(
          all_solution_map.get((problem['id'], seed)) or
          {'problemId': problem['id'], 'seed': seed, 'tag': 'nop', 'solution': ''}))

  json.dump(final_solutions, sys.stdout)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
