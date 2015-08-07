#!/bin/bash

root_dir="$(dirname "$0")/.."

dirs=(
  $root_dir/pylib
  $root_dir/third_party/python-gflags-2.0
  $root_dir/third_party/bottle-0.12.8
  ${PYTHONPATH:+"$PYTHONPATH"}
)

IFS=:
export PYTHONPATH="${dirs[@]}"
unset IFS

exec python "$@"
