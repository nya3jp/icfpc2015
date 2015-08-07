#!/bin/bash

root_dir="$(dirname "$0")/.."

export PYTHONPATH="$root_dir/pylib:$root_dir/third_party/python-gflags:${PYTHONPATH:+":$PYTHONPATH"}"
exec python "$@"
