import sys

import bottle
import gflags
import pymongo

FLAGS = gflags.FLAGS

gflags.DEFINE_integer('port', None, 'port')
gflags.MarkFlagAsRequired('port')


@bottle.get('/')
def index_handler():
  return 'This is a solution logging server.'


@bottle.post('/log')
def log_handler():
  solutions = bottle.request.json
  if not solutions:
    return bottle.abort(400, 'Malformed request (not a JSON)')
  if not isinstance(solutions, list):
    return bottle.abort(400, 'Malformed solutions (top-level not a list)')
  for solution in solutions:
    db.insert(solution)


def main(unused_argv):
  global db
  db = pymongo.MongoClient().natsubate.solutions
  bottle.run(host='0.0.0.0', port=FLAGS.port, quiet=True)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
