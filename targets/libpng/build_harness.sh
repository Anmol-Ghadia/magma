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
export AFL_DEBUG=1

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
			  LINK_CC="$CC"
			  ;;
		  cc|cpp)
			  COMPILE_CC="$RAW_CXX"
			  LINK_CC="$CXX"
			  ;;
		  *)
			  echo "Unknown extension for $HARNESS, skipping"
			  continue
			  ;;
	  esac

	  "$COMPILE_CC" -I. -c "$HARNESS" -o "$OUT/$NAME.o"
	  "$LINK_CC" "$OUT/$NAME.o" -o "$OUT/$NAME" \
		  -Wl,--whole-archive .libs/libpng16.a -Wl,--no-whole-archive \
		  $LDFLAGS $LIBS -lz
	  done

else
  echo "using OSS-FUZZ harness"
  # build libpng_read_fuzzer.
  $CXX $CXXFLAGS -std=c++11 -I. \
       contrib/oss-fuzz/libpng_read_fuzzer.cc \
       -o $OUT/libpng_read_fuzzer \
       $LDFLAGS .libs/libpng16.a $LIBS -lz
fi
