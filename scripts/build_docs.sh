#!/bin/bash

set -euxo pipefail

do_push=$1

proj_dir=$(cd "$(dirname "$0")/.."; pwd)

cd "$proj_dir"
doxygen

if [ "$do_push" != "true" ]; then
    exit 0
fi

temp_dir=$(mktemp -d -t libaramid-docs-XXXXXXXXXX)
cleanup_temp_dir() {
    rm -rf "$temp_dir"
}
trap cleanup_temp_dir EXIT

cp -rT "$proj_dir"/doxygen_html "$temp_dir"
cp .gitattributes "$temp_dir"

git fetch
git reset --hard
git clean -xdf
git checkout gh-pages
git pull
git rm -rf .

touch .nojekyll
cp -rT "$temp_dir" "$proj_dir"

git add .

git_user_email="libaramid-githubactions@nontan.dev"
git_user_name="libaramid GitHub Actions Bot"
git commit --allow-empty -m "Build documentation for $GITHUB_SHA" --author="$git_user_name <$git_user_email>"

git push -f
