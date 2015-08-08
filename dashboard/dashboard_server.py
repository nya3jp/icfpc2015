import collections
import json
import os
import sys

import bottle
import gflags
import pymongo

FLAGS = gflags.FLAGS

gflags.DEFINE_integer('port', None, 'port')
gflags.MarkFlagAsRequired('port')

bottle.TEMPLATE_PATH = [os.path.join(os.path.dirname(__file__), 'views')]


def ensure_index():
  db.problems.ensure_index([('id', pymongo.ASCENDING)])


def init_problems():
  problems_dir = os.path.join(os.path.dirname(__file__), '..', 'problems')
  for name in os.listdir(problems_dir):
    path = os.path.join(problems_dir, name)
    if path.endswith('.json'):
      with open(path) as f:
        problem = json.load(f)
      db.problems.update({'id': problem['id']}, problem, upsert=True)


@bottle.get('/')
def index_handler():
  problem_ids = []
  problem_seed_sizes = {}
  for problem in db.problems.find(sort=[('id', pymongo.ASCENDING)]):
    problem_id = problem['id']
    problem_ids.append(problem_id)
    problem_seed_sizes[problem_id] = len(problem['sourceSeeds'])
  best_seed_solution_map = collections.defaultdict(
    lambda: {'tag': 'nop', '_score': 0})
  for solution in db.solutions.find():
    key = (solution['problemId'], solution['seed'], solution['tag'])
    if solution.get('_score', -1) > best_seed_solution_map[key]['_score']:
      best_seed_solution_map[key] = solution
  tags = sorted(set(tag for (_, _, tag) in best_seed_solution_map.iterkeys()))
  best_problem_solution_map = collections.defaultdict(
    lambda: {'_solutions': [], '_avg_score': 0})
  for (problem_id, seed, tag), solution in best_seed_solution_map.iteritems():
    best_problem_solution_map[(problem_id, tag)]['_solutions'].append(solution)
  for (problem_id, tag), entry in best_problem_solution_map.iteritems():
    entry['_avg_score'] = (
      0 if not entry['_solutions']
      else
      float(sum(s['_score'] for s in entry['_solutions'])) / problem_seed_sizes[problem_id])
  template_values = {
    'problem_ids': problem_ids,
    'tags': tags,
    'best_solution_map': best_problem_solution_map,
  }
  return bottle.template('index.html', **template_values)


@bottle.get('/state-of-the-art.json')
def state_of_the_art_json_handler():
  best_solution_map = {}
  for solution in db.solutions.find():
    key = (solution['problemId'], solution['seed'])
    if (key not in best_solution_map or
        solution.get('_score', -1) > best_solution_map[key]['_score']):
      best_solution_map[key] = solution
  solutions = list(best_solution_map.values())
  for solution in solutions:
    solution.pop('_id', None)
    solution.pop('_processed', None)
  bottle.response.content_type = 'application/json'
  bottle.response.headers['Access-Control-Allow-Origin'] = '*'
  bottle.response.headers['Access-Control-Allow-Methods'] = 'GET, OPTIONS'
  bottle.response.headers['Access-Control-Allow-Headers'] = 'Origin, Accept, X-Requested-With'
  return json.dumps(solutions)


def main(unused_argv):
  global db
  db = pymongo.MongoClient().natsubate
  ensure_index()
  init_problems()
  bottle.run(host='0.0.0.0', port=FLAGS.port, quiet=True)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
