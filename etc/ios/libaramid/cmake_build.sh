#!/bin/sh

#  cmake_build.sh
#  libaramid
#
#  Created by nontan-rh on 2020/03/14.
#  Copyright © 2020 nontan-rh. All rights reserved.

set -euxo pipefail

WORKING_DIR="$DERIVED_FILE_DIR/cmake_build"
mkdir -p $WORKING_DIR
cd $WORKING_DIR

echo "Running in $WORKING_DIR"

REPO_ROOT=$(cd "$SRCROOT/../../../"; pwd)

case "$PLATFORM_NAME" in
	iphoneos)
		PLATFORM='OS64'
		;;
	iphonesimulator)
		PLATFORM='SIMULATOR64'
		;;
	*)
		echo "Unknown PLATFORM_NAME: $PLATFORM_NAME"
		exit 1
		;;
esac

case "$CONFIGURATION" in
	Debug)
		BUILD_TYPE='Debug'
		;;
	Release)
		BUILD_TYPE='Release'
		;;
	*)
		echo "Unknown CONFIGURATION: $CONFIGURATION"
		exit 1
		;;
esac

cmake $REPO_ROOT \
	-G 'Unix Makefiles' \
	-DCMAKE_TOOLCHAIN_FILE="$REPO_ROOT/cmake/toolchains/ios.toolchain.cmake" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DENABLE_VISIBILITY=ON \
	-DENABLE_BITCODE=ON \
	-DENABLE_ARC=ON \
	-DENABLE_STRICT_TRY_COMPILE=ON \
	-DPLATFORM="$PLATFORM"

cmake --build .
