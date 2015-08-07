#!/bin/bash

root_dir="$(dirname "$0")/.."

export PYTHONPATH="$root_dir/pylib:$root_dir/third_party/python-gflags-2.0:${PYTHONPATH:+":$PYTHONPATH"}"
exec python "$@"
