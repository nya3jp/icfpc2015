import argparse

import manual_submit

_PREFIX = (
    'bbbblklppappkaaplpkpaaapplpplpllllbbdblblppaakaaaaaplaaaaaaaaaalaa'
    'laalllallbldlllllbdbllllllalallapaaaalalaadppalaaaalaaaaaallalaala'
    'alllbdbllll ')


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', required=True)
    parser.add_argument('--phrase', action='append', required=True)

    parser.add_argument('--dry-run', action='store_true')
    return parser.parse_args()


def main():
    parsed_args = _parse_args()
    # NOTE: '\n' is ignored.
    parsed_args.solution = _PREFIX + '\n'.join(parsed_args.phrase)
    parsed_args.problem = 22
    parsed_args.seed = 0
    manual_submit.run(parsed_args)


if __name__ == '__main__':
    main()
