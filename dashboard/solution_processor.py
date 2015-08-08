import logging
import sys
import time

import logging_util
import gflags
import pymongo

FLAGS = gflags.FLAGS

PROCESSOR_VERSION = 1


def process_solution(db, solution):
  problem_id = solution['problemId']
  seed = solution['seed']
  tag = solution['tag']
  query = {'problemId': problem_id, 'seed': seed}
  logging.info(
    'Processing solution: problem %s, seed %s, tag %s', problem_id, seed, tag)
  solution['_processed'] = PROCESSOR_VERSION
  best_solution = db.best_solutions.find_one(query)
  if not best_solution or solution['_score'] > best_solution['_score']:
    logging.info(
      'New record: problem %s, seed %s: score %s',
      problem_id, seed, solution['_score'])
    db.best_solutions.update(query, solution, upsert=True)
  db.solutions.update(query, {'$set': {'_processed': PROCESSOR_VERSION}})


def main(unused_argv):
  logging_util.setup()

  db = pymongo.MongoClient().natsubate
  db.solutions.ensure_index([('_processed', pymongo.ASCENDING)])
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
