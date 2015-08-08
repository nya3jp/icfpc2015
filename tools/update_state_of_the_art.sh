#!/bin/bash

cd "$(dirname "$0")/.."

exec wget -O solutions/state-of-the-art.json http://dashboard.natsubate.nya3.jp/state-of-the-art.json
