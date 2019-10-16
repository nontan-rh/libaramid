#!/bin/bash

set -euxo pipefail

tag=$1
do_push=$2

cd "$(dirname "$0")/../docker"

registry='nontanrh'
base_name_tag="$registry/libaramid-ci-base:$tag"

docker build . \
    --file "libaramid-ci-base.dockerfile" \
    --tag "$base_name_tag"
if [ "$do_push" = "true" ]; then
    docker push "$base_name_tag"
fi

build_variation() {
    local variation=$1
    local name="libaramid-ci-$variation"
    docker build . \
        --file "$name.dockerfile" \
        --build-arg "base=$base_name_tag" \
        --tag "$registry/$name:$tag"
    if [ "$do_push" = "true" ]; then
        docker push "$registry/$name:$tag"
    fi
}

build_variation 'emscripten'
build_variation 'android'
build_variation 'clang'
build_variation 'nodejs'
build_variation 'doxygen'
