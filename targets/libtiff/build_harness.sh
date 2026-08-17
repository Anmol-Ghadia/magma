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
cd "$TARGET/repo"

if [ ! -z "$HARNESSES" ]; then
  HARNESS_DIR="$TARGET/$HARNESSES"

  # TODO(Mayant): Do I want to keep this configurable? I need a non-AFL C compiler
  # here, OR use an instrumentation denylist.
  RAW_CC="clang"
  RAW_CXX="clang++"

  if [ ! -d "$HARNESS_DIR" ]; then
    echo "harness directory $HARNESS_DIR does not exist."
    exit 1
  fi

  echo "Building custom harnesses"
  for HARNESS in $HARNESS_DIR/*.c; do
	  NAME=$(basename $HARNESS .c)
	  $RAW_CC -I"$WORK/include" -I. \
		  -I"$HARNESS_DIR" \
		  -c $HARNESS -o "$OUT/$NAME.o"
	  $CC "$OUT/$NAME.o" -o "$OUT/$NAME" \
		  -Wl,--whole-archive \
		  $WORK/lib/libtiffxx.a $WORK/lib/libtiff.a \
		  -Wl,--no-whole-archive \
		  -lz -ljpeg -Wl,-Bstatic -llzma -Wl,-Bdynamic \
		  $LDFLAGS $LIBS
  done

else
  echo "Harness missing"
  exit 1
fi
