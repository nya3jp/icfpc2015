import argparse
import json
import os
import subprocess
import tempfile


def _encode_json(problem, seed, tag, solution):
    value = [{
        'problemId': problem,
        'seed': seed,
        'tag': tag,
        'solution': solution
    }]
    return json.dumps(value, separators=(',', ':'))


def _local_score(problem, seed, tag, solution):
    with tempfile.NamedTemporaryFile(delete=False) as stream:
        try:
            stream.write(_encode_json(problem, seed, tag, solution))
            stream.close()

            print 'Score:'
            subprocess.check_call([
                './simulator/scorer',
                '--problem', 'problems/problem_%d.json' % problem,
                '--output', stream.name])
        finally:
            os.unlink(stream.name)


def _submit(problem, seed, tag, solution):
    subprocess.check_call([
        'curl', '--user', ':5HCRz0UOZSsseufyW36DvmeWJKUS9mMPf1qXaAuGM9g=',
        '--header', 'Content-Type: application/json',
        '--data', _encode_json(problem, seed, tag, solution),
        'https://davar.icfpcontest.org/teams/116/solutions'])


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--problem', type=int, required=True)
    parser.add_argument('--seed', type=int, required=True)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--solution', required=True)

    return parser.parse_args()


def main():
    parsed_args = _parse_args()
    _local_score(parsed_args.problem, parsed_args.seed,
                 parsed_args.tag, parsed_args.solution)
    _submit(parsed_args.problem, parsed_args.seed,
            parsed_args.tag, parsed_args.solution)


if __name__ == '__main__':
    main()
