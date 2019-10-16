#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

status=0

check() {
    for file in "$@"; do
        set +e
        npx cspell $file
        file_status=$?
        set -e
        if [ $file_status != 0 ]; then
            status=1
        fi
    done
}

check "$proj_dir"/**/*.h
check "$proj_dir"/**/*.hpp
check "$proj_dir"/**/*.c
check "$proj_dir"/**/*.cpp
check "$proj_dir"/**/*.mm
check "$proj_dir"/**/*.java
check "$proj_dir"/**/*.sh
check "$proj_dir"/**/*.md
check "$proj_dir"/LICENSE.txt

exit $status
