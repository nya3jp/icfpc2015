#!/usr/bin/python

import logging
import sys
import time

import logging_util
import gflags
import pymongo
import requests
import ujson as json

FLAGS = gflags.FLAGS


def main(unused_argv):
  logging_util.setup()

  db = pymongo.MongoClient().natsubate
  db.leaderboard.ensure_index([('time', pymongo.ASCENDING)])

  while True:
    try:
      content = requests.get('https://davar.icfpcontest.org/rankings.js').content
      ranking = json.loads(content.split(' = ', 1)[1])
      db.leaderboard.update({'time': ranking['time']}, ranking, upsert=True)
      logging.info('%s', ranking['time'])
    except Exception:
      logging.exception('Uncaught exception')
    time.sleep(60)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
