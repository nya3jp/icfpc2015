import glob
import json
import os
import sys


def main():
    if len(sys.argv) != 2:
        print 'Usage: %s [problems dir]' % sys.argv[0]
        sys.exit(2)

    problems_dir = sys.argv[1]
    max_width = 0
    max_height = 0
    for path in sorted(glob.iglob(os.path.join(problems_dir, '*'))):
        with open(path) as stream:
            data = json.load(stream)
        width = int(data['width'])
        height = int(data['height'])
        print '%s: %d, %d' % (path, width, height)
        max_width = max(max_width, width)
        max_height = max(max_height, height)
    print 'Max size: %d, %d' % (max_width, max_height)


if __name__ == '__main__':
    main()
