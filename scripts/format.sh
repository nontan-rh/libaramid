#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)
clang-format-9 -i \
    "$proj_dir"/lib/src/*.c \
    "$proj_dir"/lib/src/*.cpp \
    "$proj_dir"/lib/src/*.h \
    "$proj_dir"/lib/include/aramid/*.h \
    "$proj_dir"/tests/src/*.cpp
