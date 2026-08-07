#!/bin/bash

##
# Pre-requirements:
# - env TARGET: path to target work dir
##

git clone --no-checkout https://github.com/intel/tinycbor.git \
    "$TARGET/repo"
git -C "$TARGET/repo" checkout 49d3a238cf4b7b7ff8cba1836803af60ca9c7dc5
