#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)
prefix_dir="$proj_dir/local"

rm -fr "$proj_dir/build"
mkdir -p "$proj_dir/build"
cd "$proj_dir/build"

rm -fr "$prefix_dir"
mkdir -p "$prefix_dir"

cmake .. "$@" -DCMAKE_INSTALL_PREFIX="$prefix_dir"
cmake --build .
cmake --build . --target install

rm -fr "$proj_dir/build_consumer"
mkdir -p "$proj_dir/build_consumer"
cd "$proj_dir/build_consumer"

cmake ../etc/consumer -DCMAKE_PREFIX_PATH="$prefix_dir" -DCMAKE_FIND_DEBUG_MODE=ON
cmake --build .
