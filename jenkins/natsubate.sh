#!/bin/bash

export LANG=C
cd "$(dirname "$0")/.."
set -ex


# Update dashboard

args=()

for i in $(seq 0 99); do
  path="problems/problem_$i.json"
  if [[ -f "$path" ]]; then
    args+=(-f $path)
  fi
done

tools/python.sh dashboard/update.py ${args[@]} --output_dir=out/dashboard --logtostderr=info


# Submit to leaderboard

api_key_path=/tmp/api_key.txt
if [[ -f $api_key_path ]]; then
  tools/python.sh dashboard/submit.py --logtostderr=info --api_key="$(cat $api_key_path)" --team_id=116 out/dashboard/solution_*_BEST.json
fi
