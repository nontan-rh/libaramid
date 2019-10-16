#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

mkdir -p "$proj_dir/build"
cd "$proj_dir/build"

cmake .. "$@"
cmake --build .
ctest --verbose
