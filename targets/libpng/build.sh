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
autoreconf -f -i
./configure --with-libpng-prefix=MAGMA_ --disable-shared
echo "building libpng with CC=($CC) and CXX=($CXX)"
export AFL_DEBUG=1
make -j$(nproc) clean
script -q -e -c "make libpng16.la" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

cp .libs/libpng16.a "$OUT/"

if [ ! -z "$HARNESSES" ]; then
  HARNESS_DIR="$TARGET/$HARNESSES"

  # TODO(Mayant): Do I want to keep this configurable? I need a non-AFL C compiler
  # here, OR use an instrumentation denylist.
  RAW_CC="clang"

  if [ ! -d "$HARNESS_DIR" ]; then
    echo "harness directory $HARNESS_DIR does not exist."
    exit 1
  fi

  # TODO: find a better fix
  # both the lines below remove libAFLDriver from the archive as it should only
  # be linked once at the final stage when linking the harness.
  # The exported $LIBS currently also adds libAFLDriver archive inside the
  # library archive so we get error when compiling the harness
  ar d .libs/libpng16.a libAFLDriver.a

  echo "Building custom harnesses"
  for HARNESS in $HARNESS_DIR/*.c; do
    NAME=$(basename $HARNESS .c)
    $RAW_CC -I. -c $HARNESS -o "$OUT/$NAME.o"
    $CC "$OUT/$NAME.o" -o "$OUT/$NAME" \
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
