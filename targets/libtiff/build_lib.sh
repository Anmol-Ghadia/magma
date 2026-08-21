#!/bin/bash
set -e

##
# Pre-requirements:
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env CC, CXX, FLAGS, LIBS, etc...
##

if [ ! -d "$TARGET/repo" ]; then
    echo "fetch.sh must be executed first."
    exit 1
fi

WORK="$TARGET/work"
rm -rf "$WORK"
mkdir -p "$WORK"
mkdir -p "$WORK/lib" "$WORK/include"

cd "$TARGET/repo"
./autogen.sh
./configure --disable-shared --enable-static --prefix="$WORK"
make -j$(nproc) clean
script -q -e -c "make install" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

