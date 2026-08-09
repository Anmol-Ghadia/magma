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
rm -rf "$WORK"
mkdir -p "$WORK"
cd "$WORK"

export CFLAGS="$CFLAGS -DSQLITE_MAX_LENGTH=128000000 \
               -DSQLITE_MAX_SQL_LENGTH=128000000 \
               -DSQLITE_MAX_MEMORY=25000000 \
               -DSQLITE_PRINTF_PRECISION_LIMIT=1048576 \
               -DSQLITE_DEBUG=1 \
               -DSQLITE_MAX_PAGE_COUNT=16384"

"$TARGET/repo"/configure --disable-shared --enable-rtree
make clean
script -q -e -c "make" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

cp sqlite3.o "$OUT/"

#$CC $CFLAGS -I. \
#    "$TARGET/repo/test/ossfuzz.c" "./sqlite3.o" \
#    -o "$OUT/sqlite3_fuzz" \
#    $LDFLAGS $LIBS -pthread -ldl -lm

# =========

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
