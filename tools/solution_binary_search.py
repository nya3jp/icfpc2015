#!/usr/bin/python

import json
import logging
import os
import subprocess
import sys
import tempfile
import time

import gflags
import logging_util
import requests

FLAGS = gflags.FLAGS

gflags.DEFINE_string('problem', None, '')
gflags.DEFINE_string('output', None, '')
gflags.DEFINE_string('scorer', None, '')
gflags.DEFINE_integer('good', None, '')
gflags.DEFINE_integer('bad', None, '')
gflags.MarkFlagAsRequired('problem')
gflags.MarkFlagAsRequired('output')
gflags.MarkFlagAsRequired('scorer')


def main(unused_argv):
  logging_util.setup()

  with open(FLAGS.problem) as f:
    problem = json.load(f)
  with open(FLAGS.output) as f:
    solutions = json.load(f)
  del solutions[1:]
  first_solution = solutions[0]
  for seed in problem['sourceSeeds']:
    if seed != first_solution['seed']:
      solutions.append({'problemId': problem['id'], 'seed': seed, 'tag': 'empty', 'solution': ''})
  assert len(solutions) == len(problem['sourceSeeds'])
  full_sequence = first_solution['solution']
  good = FLAGS.good if FLAGS.good is not None else 0
  bad = FLAGS.bad if FLAGS.bad is not None else len(full_sequence)
  while bad - good > 1:
    middle = (good + bad) / 2
    tag = 'bin%d' % middle
    first_solution['solution'] = full_sequence[:middle]
    first_solution['tag'] = tag
    with tempfile.NamedTemporaryFile() as f:
      json.dump(solutions, f)
      f.flush()
      with open(os.devnull, 'w') as devnull:
        output = subprocess.check_output([FLAGS.scorer, '--problem=%s' % FLAGS.problem, '--output=%s' % f.name], stderr=devnull)
    scores = map(int, output.split())
    expected_score = sum(scores) / len(scores)
    first_solution['_score'] = expected_score
    print '======= good=%d, bad=%d, middle=%d, posting following solution:' % (good, bad, middle)
    json.dump(solutions, sys.stdout)
    sys.stdout.write('\n')
    requests.post(
      'https://davar.icfpcontest.org/teams/116/solutions',
      headers={'Content-Type': 'application/json'},
      auth=('', '5HCRz0UOZSsseufyW36DvmeWJKUS9mMPf1qXaAuGM9g='),
      data=json.dumps(solutions))
    while True:
      try:
        content = requests.get('https://davar.icfpcontest.org/rankings.js').content
        leaderboard = json.loads(content.split(' = ', 1)[1])
      except Exception:
        logging.exception('Uncaught exception')
      else:
        got_score = None
        for setting in leaderboard['data']['settings']:
          if setting['setting'] == problem['id']:
            for team in setting['rankings']:
              if team['teamId'] == 116:
                if tag in team['tags']:
                  got_score = team['score']
        if got_score is not None:
          if got_score == expected_score:
            good = middle
          else:
            bad = middle
          break
      time.sleep(60)
    print 'expected %d, got %d' % (expected_score, got_score)
  print '*** good=%d, bad=%d' % (good, bad)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
