#!/bin/bash

export LANG=C
cd "$(dirname "$0")/.."
set -ex

if [[ "--nobuild" == "$1" ]]; then
  shift 1
else
  touch .deps
  make build
fi

mkdir -p out/

power_args=()
exec 3< power_phrases.txt
while read -u 3 -r i; do
  power_args+=(-p)
  power_args+=("$i")
done
exec 3<&-

time ./play_icfp2015 \
  --logtostderr=debug \
  --disable_cgroup --show_scores --report --report_tag=shinku \
  --nouse_state_of_the_art \
  -t 60 -m 1024 -c 4 \
  "${power_args[@]}" \
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
  "$@" \
  > out/output.json
