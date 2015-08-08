#!/bin/bash

cd "$(dirname "$0")/.."

args=()

for i in problems/problem_*.json; do
  args+=(-f $i)
done

set -ex

make
./play_icfp2015 "${args[@]}"
echo
