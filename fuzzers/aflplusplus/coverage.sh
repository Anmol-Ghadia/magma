#!/bin/bash

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env SHARED: path to directory shared with host (to store results)
# - env PROGRAM: name of program to run (should be found in $OUT)
# - env ARGS: extra arguments to pass to the program
# - env FUZZARGS: extra arguments to pass to the fuzzer
# - env SEEDS: seeds to start fuzzing with
##

mkdir -p "$SHARED/coverage"

# log total number of edges
if [ -e "$OUT/afl/build_output.log" ]; then
    # compute instrumented edges
    grep -o "Instrumented [[:digit:]]*" \
	    "$OUT/afl/build_output.log" | \
	    grep -o "[[:digit:]]*" | \
	    awk '{sum += $1 } END {print sum}' > \
	    "$SHARED/coverage/instrumented_edges.txt"
fi

raw_map="$SHARED/coverage/map.raw"
sorted_map="$SHARED/coverage/map.sorted"

sleep $SLEEP_TIME
"$FUZZER/repo/afl-showmap" -e -C \
	-o  "$raw_map"\
	-i "$SHARED/findings/default/queue/" \
	-- "$OUT/afl/$PROGRAM" @@ > /dev/null

sort -u "$raw_map" -o "$sorted_map"
