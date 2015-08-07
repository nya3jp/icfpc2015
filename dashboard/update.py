import glob
import json
import logging
import os
import subprocess
import sys

import logging_util
import gflags

FLAGS = gflags.FLAGS

gflags.DEFINE_multistring('problem', None, 'path to problem.json', short_name='f')
gflags.DEFINE_string('output_dir', None, 'path to output dir')
gflags.MarkFlagAsRequired('problem')
gflags.MarkFlagAsRequired('output_dir')


RUNNER_NAME = 'play_icfp2015'


def mkdir_p(path):
  subprocess.check_call(['mkdir', '-p', path])


def prepare_ais():
  ai_dirs = []
  ai_root = os.path.join(os.path.dirname(__file__), '..', 'ai')
  for ai_name in sorted(os.listdir(ai_root)):
    ai_dir = os.path.join(ai_root, ai_name)
    if not os.path.isdir(ai_dir):
      continue
    if not os.path.exists(os.path.join(ai_dir, 'Makefile')):
      continue
    logging.info('Found AI: %s', ai_name)
    returncode = subprocess.call(['make'], cwd=ai_dir)
    if returncode != 0:
      logging.error('make failed with returncode=%d', returncode)
      continue
    runner_path = os.path.join(ai_dir, RUNNER_NAME)
    if (not os.path.exists(runner_path) or
        (os.stat(runner_path).st_mode & 0100) != 0100):
      logging.error('make did not create executable play_icfp2015')
      continue
    ai_dirs.append(ai_dir)
  return ai_dirs


def main(argv):
  logging_util.setup()

  mkdir_p(FLAGS.output_dir)

  problem_paths = list(FLAGS.problem)
  ai_dirs = prepare_ais()
  ai_names = [os.path.basename(ai_dir) for ai_dir in ai_dirs]

  problems = []
  for problem_path in problem_paths:
    with open(problem_path) as f:
      problem = json.load(f)
    problem['path'] = problem_path
    problems.append(problem)

  result_map = {}
  for problem in problems:
    problem_id = problem['id']
    seeds = problem['sourceSeeds']
    for ai_dir in ai_dirs:
      ai_name = os.path.basename(ai_dir)
      runner_path = os.path.join(ai_dir, RUNNER_NAME)
      logging.info('Running AI %s for problem %d', ai_name, problem_id)
      try:
        with open(os.devnull) as devnull:
          output = subprocess.check_output(
            [runner_path, '-f', problem['path']],
            stdin=devnull)
        problem_result = json.loads(output)
      except Exception:
        logging.exception(
          'An unexpected exception raised during running AI %s', ai_name)
        problem_result = [
          {'problemId': problem_id,
           'seed': seed,
           'tag': '%s (crashed)' % ai_name,
           'solution': [],
           '_score': -1}
          for seed in seeds]
      else:
        logging.info(
          'AI finished. score: %s',
          ', '.join(str(seed_result['_score']) for seed_result in problem_result))
      for seed_result in problem_result:
        seed_result['_ai_name'] = ai_name
      with open(os.path.join(FLAGS.output_dir, 'solution_%d_%s.json' % (problem_id, ai_name)), 'w') as f:
        json.dump(problem_result, f, indent=2)
      for seed_result in problem_result:
        result_map[(problem_id, seed_result['seed'], ai_name)] = seed_result

  for problem in problems:
    problem_id = problem['id']
    seeds = problem['sourceSeeds']
    select_ais = set()
    problem_best_result = []
    for seed in seeds:
      best_seed_result = max(
        (result_map[(problem_id, seed, ai_name)] for ai_name in ai_names),
        key=lambda result: result['_score'])
      problem_best_result.append(best_seed_result)
    problem_score = float(sum(seed_best_result['_score'] for seed_best_result in problem_best_result)) / len(seeds)
    logging.info('Problem %d score: %.1f', problem_id, problem_score)
    with open(os.path.join(FLAGS.output_dir, 'solution_%d_BEST.json' % problem_id), 'w') as f:
      json.dump(problem_best_result, f, indent=2)


if __name__ == '__main__':
  sys.exit(main(FLAGS(sys.argv)))
