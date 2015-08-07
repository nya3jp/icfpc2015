#!/bin/bash

export LANG=C

cd "$(dirname "$0")/.."

args=()

for i in $(seq 0 99); do
  path="problems/problem_$i.json"
  if [[ -f "$path" ]]; then
    args+=(-f $path)
  fi
done

exec tools/python.sh dashboard/update.py ${args[@]} --output_dir=out/dashboard --logtostderr=info
