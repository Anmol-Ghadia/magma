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
cd "$TARGET/repo"

export CFLAGS="$CFLAGS -DSQLITE_MAX_LENGTH=128000000 \
               -DSQLITE_MAX_SQL_LENGTH=128000000 \
               -DSQLITE_MAX_MEMORY=25000000 \
               -DSQLITE_PRINTF_PRECISION_LIMIT=1048576 \
               -DSQLITE_DEBUG=1 \
               -DSQLITE_MAX_PAGE_COUNT=16384 \
	       -DSQLITE_ALLOW_URI_AUTHORITY \
	       -DSQLITE_ENABLE_API_ARMOR \
	       -DSQLITE_ENABLE_COLUMN_METADATA \
	       -DSQLITE_ENABLE_NORMALIZE \
	       -DSQLITE_ENABLE_PREUPDATE_HOOK \
	       -DSQLITE_ENABLE_SNAPSHOT \
	       -DSQLITE_ENABLE_STMT_SCANSTATUS \
	       -DSQLITE_ENABLE_UNLOCK_NOTIFY \
	       -DSQLITE_ENABLE_SESSION"

"$TARGET/repo"/configure --disable-shared \
	--enable-rtree \
	--prefix="$WORK" \
	--enable-session
make clean
script -q -e -c "make install" "$OUT/build_output.log"
#cp "$TARGET/repo/ext/session/sqlite3session.h" "$WORK/include/sqlite3session.h"
# script instead of direct make because afl does not print the # of instrumented
# locations unless a stderr is attached

