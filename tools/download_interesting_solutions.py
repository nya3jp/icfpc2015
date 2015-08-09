#!/usr/bin/python

import os
import subprocess
import sys

import gflags
import logging_util
import requests
import ujson as json

FLAGS = gflags.FLAGS

gflags.DEFINE_string('output_dir', None, '')
gflags.MarkFlagAsRequired('output_dir')


def main(unused_argv):
  logging_util.setup()

  subprocess.check_call(['mkdir', '-p', FLAGS.output_dir])

  best_solutions = requests.get(
    'http://dashboard.natsubate.nya3.jp/state-of-the-art.json').json()
  all_solutions = requests.get(
    'http://dashboard.natsubate.nya3.jp/all-solutions.json').json()

  best_solution_map = {}
  for solution in best_solutions:
    best_solution_map[(solution['problemId'], solution['seed'])] = solution

  interesting_solutions = []
  good_solution_map = {}
  for solution in all_solutions:
    if solution['problemId'] in (24, 178116):
      continue
    if solution['tag'] == 'rewrakkuma':
      continue
    if solution['tag'] == 'handplay_viz':
      interesting_solutions.append(solution)
      continue
    key = (solution['problemId'], solution['seed'], solution['tag'])
    if (key not in good_solution_map or
        solution['_score'] > good_solution_map[key]['_score']):
      good_solution_map[key] = solution
  interesting_solutions.extend(good_solution_map.values())

  for solution in interesting_solutions:
    solution['_target_score'] = best_solution_map[
      (solution['problemId'], solution['seed'])]['_score']

  for i, solution in enumerate(interesting_solutions):
    json_path = os.path.join(FLAGS.output_dir, '%08d.json' % i)
    with open(json_path, 'w') as f:
      json.dump([solution], f)
    print 'saved', json_path


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
