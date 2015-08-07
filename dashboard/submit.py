import json
import logging
import subprocess
import sys
import tempfile

import logging_util
import gflags

FLAGS = gflags.FLAGS

gflags.DEFINE_string('api_key', None, 'API key')
gflags.DEFINE_string('team_id', None, 'team ID')
gflags.MarkFlagAsRequired('api_key')
gflags.MarkFlagAsRequired('team_id')


def main(argv):
  logging_util.setup()

  for solution_path in argv[1:]:
    logging.info('Submitting: %s', solution_path)
    with open(solution_path) as f:
      problem_result = json.load(f)
    for seed_result in problem_result:
      for key in seed_result.keys():
        if key.startswith('_'):
          del seed_result[key]
    with tempfile.NamedTemporaryFile() as f:
      json.dump(problem_result, f, indent=2)
      f.flush()
      subprocess.check_call([
        'curl', '--user', ':%s' % FLAGS.api_key,
        '--header', 'Content-Type: application/json',
        '--data', '@%s' % f.name,
        'https://davar.icfpcontest.org/teams/%s/solutions' % FLAGS.team_id])


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
