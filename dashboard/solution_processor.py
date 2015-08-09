#!/usr/bin/python

import json
import logging
import os
import subprocess
import sys
import tempfile
import time

import logging_util
import gflags
import pymongo

FLAGS = gflags.FLAGS

gflags.DEFINE_string('scorer', None, 'scorer path')
gflags.MarkFlagAsRequired('scorer')

PROCESSOR_VERSION = 6


def init_problems(db):
  problems_dir = os.path.join(os.path.dirname(__file__), '..', 'problems')
  for name in os.listdir(problems_dir):
    path = os.path.join(problems_dir, name)
    if path.endswith('.json'):
      with open(path) as f:
        problem = json.load(f)
      db.problems.update({'id': problem['id']}, problem, upsert=True)


def ensure_index(db):
  db.solutions.ensure_index([('_processed', pymongo.ASCENDING)])


def compute_score(db, solution):
  problem = db.problems.find_one({'id': solution['problemId']})
  if not problem:
    logging.error('Unknown problem %d', solution['problemId'])
    return -1
  problem_json = problem.copy()
  del problem_json['_id']
  solution_json = solution.copy()
  del solution_json['_id']
  with tempfile.NamedTemporaryFile() as problem_f:
    json.dump(problem_json, problem_f)
    problem_f.flush()
    with tempfile.NamedTemporaryFile() as solution_f:
      json.dump([solution_json], solution_f)
      solution_f.flush()
      with open(os.devnull, 'w') as devnull:
        output = subprocess.check_output(
          [FLAGS.scorer,
           '--problem=%s' % problem_f.name,
           '--output=%s' % solution_f.name],
          stderr=devnull)
  return int(output.strip())


def process_solution(db, solution):
  problem_id = solution['problemId']
  seed = solution['seed']
  tag = solution['tag']
  task_query = {'problemId': problem_id, 'seed': seed}
  score = compute_score(db, solution)
  solution['_score'] = score
  solution['_processed'] = PROCESSOR_VERSION
  logging.info(
    'Processed: problem %s, seed %s, tag %s: score %s', problem_id, seed, tag, score)
  db.solutions.save(solution)


def main(unused_argv):
  logging_util.setup()

  db = pymongo.MongoClient().natsubate
  init_problems(db)
  ensure_index(db)

  while True:
    solution = db.solutions.find_one({'_processed': None})
    if not solution:
      solution = db.solutions.find_one({'_processed': {'$lt': PROCESSOR_VERSION}})
    if not solution:
      time.sleep(3)
      continue
    try:
      process_solution(db, solution)
    except Exception:
      logging.exception('An unexpected exception during processing: %r' % solution)
      db.solutions.update(
        {'_id': solution['_id']},
        {'$set': {'_processed': PROCESSOR_VERSION}})


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
