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
  for HARNESS in "$HARNESS_DIR"/*.c "$HARNESS_DIR"/*.cc "$HARNESS_DIR"/*.cpp; do
	  [ -e "$HARNESS" ] || continue   # skip if a glob pattern matched nothing

	  EXT="${HARNESS##*.}"
	  NAME=$(basename "$HARNESS" ".$EXT")

	  case "$EXT" in
		  c)
			  COMPILE_CC="$RAW_CC"
			  COMPILE_FLAGS="$CFLAGS"
			  LINK_CC="$CC"
			  ;;
		  cc|cpp)
			  COMPILE_CC="$RAW_CXX"
			  COMPILE_FLAGS="$CXXFLAGS"
			  LINK_CC="$CXX"
			  ;;
		  *)
			  echo "Unknown extension for $HARNESS, skipping"
			  continue
			  ;;
	  esac

export AFL_DEBUG=1

    $COMPILE_CC $COMPILE_FLAGS \
	    -I"$WORK/include/tinycbor" -I./src \
	    -c $HARNESS -o "$OUT/$NAME.o"
    $LINK_CC "$OUT/$NAME.o" -o "$OUT/$NAME" \
	    -Wl,--whole-archive \
	    "$WORK/lib/libtinycbor.a" \
	    -Wl,--no-whole-archive \
	    $LDFLAGS $LIBS
  done

else
  echo "Harness missing"
  exit 1
fi
