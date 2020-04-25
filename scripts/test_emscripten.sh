#!/bin/bash

set -euxo pipefail

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

/emsdk/emsdk activate latest
. /emsdk/emsdk_env.sh

rm -fr "$proj_dir/build"
mkdir -p "$proj_dir"/build
cd "$proj_dir"/build

emcmake cmake .. "$@"
cmake --build .

test_driver_dir="$proj_dir/etc/wasm/test_driver"
npm install --prefix "$test_driver_dir"

old_pwd=$(pwd)
cd test_executable
python3 -m http.server 8080 &
http_server_pid=$!
cd "$old_pwd"

stop_server() {
    kill -9 $http_server_pid
}
trap stop_server EXIT

npm start --prefix "$test_driver_dir" -- --no-sandbox --num_executors=1
npm start --prefix "$test_driver_dir" -- --no-sandbox --num_executors=2
npm start --prefix "$test_driver_dir" -- --no-sandbox --num_executors=4
