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
script -q -e -c "make" "$OUT/build_output.log"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached
make install

cp "$WORK/bin/tiffcp" "$OUT/"
# ======
#$CXX $CXXFLAGS -std=c++11 -I$WORK/include \
#    contrib/oss-fuzz/tiff_read_rgba_fuzzer.cc -o $OUT/tiff_read_rgba_fuzzer \
#    $WORK/lib/libtiffxx.a $WORK/lib/libtiff.a -lz -ljpeg -Wl,-Bstatic -llzma -Wl,-Bdynamic \
#    $LDFLAGS $LIBS
# ========

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

  # TODO: find a better fix
  # both the lines below remove libAFLDriver from the archive as it should only
  # be linked once at the final stage when linking the harness.
  # The exported $LIBS currently also adds libAFLDriver archive inside the
  # library archive so we get error when compiling the harness
  ar d "$WORK/lib/libtiffxx.a" libAFLDriver.a
  ar d "$WORK/lib/libtiff.a" libAFLDriver.a

  echo "Building custom harnesses"
  for HARNESS in $HARNESS_DIR/*.c; do
	  NAME=$(basename $HARNESS .c)
	  $RAW_CC -I"$WORK/include" -I. -c $HARNESS -o "$OUT/$NAME.o"
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
