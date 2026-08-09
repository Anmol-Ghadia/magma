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

cmake ../repo
echo "building tinycbor with CC=($CC) and CXX=($CXX). Build output at SHARED=($SHARED)"
export AFL_DEBUG=1
make -j$(nproc) clean
#make -j$(nproc) tinycbor > "$OUT/build_output.log" 2>&1
script -q -e -c "make" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

cp libtinycbor.a "$OUT/"

if [ ! -z "$HARNESSES" ]; then
  HARNESS_DIR="$TARGET/$HARNESSES"

  # TODO(Mayant): Do I want to keep this configurable? I need a non-AFL C compiler
  # here, OR use an instrumentation denylist.
  RAW_CC="clang"

  if [ ! -d "$HARNESS_DIR" ]; then
    echo "harness directory $HARNESS_DIR does not exist."
    exit 1
  fi

  echo "Building custom harnesses"
  for HARNESS in $HARNESS_DIR/*.c; do
    NAME=$(basename $HARNESS .c)
    $RAW_CC -I"$TARGET/repo/src" -I. -c $HARNESS -o "$OUT/$NAME.o"
    $CC "$OUT/$NAME.o" -o "$OUT/$NAME" \
	    -Wl,--whole-archive "$OUT/libtinycbor.a" -Wl,--no-whole-archive \
	    $LDFLAGS $LIBS
  done

else
  echo "Harness missing"
  exit 1
fi
