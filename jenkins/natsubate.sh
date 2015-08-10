#!/bin/bash

export LANG=C
cd "$(dirname "$0")/.."
set -ex

make build

mkdir -p out/

./play_icfp2015 \
  --logtostderr=debug \
  --disable_cgroup --show_scores --noreport \
  -t 60 -m 1024 -c 4 \
  -f problems/problem_0.json \
  -f problems/problem_1.json \
  -f problems/problem_2.json \
  -f problems/problem_3.json \
  -f problems/problem_4.json \
  -f problems/problem_5.json \
  -f problems/problem_6.json \
  -f problems/problem_7.json \
  -f problems/problem_8.json \
  -f problems/problem_9.json \
  -f problems/problem_10.json \
  -f problems/problem_11.json \
  -f problems/problem_12.json \
  -f problems/problem_13.json \
  -f problems/problem_14.json \
  -f problems/problem_15.json \
  -f problems/problem_16.json \
  -f problems/problem_17.json \
  -f problems/problem_18.json \
  -f problems/problem_19.json \
  -f problems/problem_20.json \
  -f problems/problem_21.json \
  -f problems/problem_22.json \
  -f problems/problem_23.json \
  -f problems/problem_24.json \
  > out/output.json
