#!/bin/bash
set -e

##
# Pre-requirements:
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env CC, CXX, FLAGS, LIBS, etc...
# + env HARNESSES: path to directory with custom harnesses (default: unset)
##

if [ ! -d "$TARGET/repo" ]; then
    echo "fetch.sh must be executed first."
    exit 1
fi

# build the libpng library
cd "$TARGET/repo"

WORK="$TARGET/temp"
rm -rf $WORK
mkdir -p $WORK
cd $WORK

cmake ../repo -DCMAKE_INSTALL_PREFIX=$WORK -DBUILD_SHARED_AND_STATIC_LIBS=ON
echo "building cjson with CC=($CC) and CXX=($CXX). Build output at SHARED=($SHARED)"
export AFL_DEBUG=1
make -j$(nproc) clean
script -q -e -c "make" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

cp libcjson.a "$OUT/"

