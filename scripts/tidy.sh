#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

rm -fr "$proj_dir/build"
mkdir -p "$proj_dir/build"
cd "$proj_dir/build"

cmake "$proj_dir" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DTIDY_MODE=ON
cmake --build .

clang-tidy-9 -p . \
    "$proj_dir"/lib/src/*.c \
    "$proj_dir"/lib/src/*.cpp \
    "$proj_dir"/lib/src/*.h \
    "$proj_dir"/lib/include/aramid/*.h \
    "$proj_dir"/tests/src/*.cpp \
    "$proj_dir"/tests/src/*.hpp \
    "$proj_dir"/test_executable/src/*.cpp \
    "$proj_dir"/test_library/include/aramid/*.hpp \
    "$proj_dir"/test_library/src/*.cpp
