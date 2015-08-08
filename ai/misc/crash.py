#!/usr/bin/python

"""Mock AI to crash with partial output JSON."""

import sys


def main(unused_argv):
  sys.stdout.write('[{"problemId":')
  return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv))
