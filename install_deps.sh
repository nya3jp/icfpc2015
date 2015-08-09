#!/bin/bash

cd "$(dirname "$0")"

set -ex

sudo apt-get install -y build-essential cmake g++ automake libunwind8-dev python-dev python-pip cgroup-bin
pip install --user -r requirements.txt
