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

# build the sqlite3 library
cd "$TARGET/repo"

export WORK="$TARGET/work"
cd "$WORK"

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
    $RAW_CC $CFLAGS -I. -c $HARNESS -o "$OUT/$NAME.o"
    $CC "$OUT/$NAME.o" -o "$OUT/$NAME" \
	    -Wl,--whole-archive "$OUT/sqlite3.o" -Wl,--no-whole-archive \
	    $LDFLAGS $LIBS -pthread -ldl -lm
  done

else
  echo "Harness missing"
  exit 1
fi
