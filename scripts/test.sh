#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

mkdir -p "$proj_dir/build"
cd "$proj_dir/build"

cmake .. "$@"
cmake --build .
ARAMID_TEST_NUM_EXECUTORS=1 ctest --verbose
ARAMID_TEST_NUM_EXECUTORS=2 ctest --verbose
ARAMID_TEST_NUM_EXECUTORS=4 ctest --verbose
