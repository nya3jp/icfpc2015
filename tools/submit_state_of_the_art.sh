#!/bin/bash

cd "$(dirname "$0")/.."

set -ex

make

for i in problems/problem_*.json; do
  ./play_icfp2015 -f $i > /tmp/submit.json
  curl --user ':5HCRz0UOZSsseufyW36DvmeWJKUS9mMPf1qXaAuGM9g=' --header 'Content-Type: application/json' --data '@/tmp/submit.json' 'https://davar.icfpcontest.org/teams/116/solutions'
  rm -f /tmp/submit.json
done
