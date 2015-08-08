#!/usr/bin/python

import json
import subprocess
import sys
import tempfile


def main():
  content = sys.stdin.read()
  sys.stdout.write(content)
  try:
    solutions = json.loads(content)
    with tempfile.NamedTemporaryFile() as f:
      f.write(content)
      f.flush()
      subprocess.check_call(
        ['curl',
         '--header', 'Content-Type: application/json',
         '--data', '@%s' % f.name,
         'http://solutions.natsubate.nya3.jp/log'])
  except Exception as e:
    print >>sys.stderr, 'An exception on logging solutions: %r' % e


if __name__ == '__main__':
  main()
