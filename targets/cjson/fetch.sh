#!/bin/bash

##
# Pre-requirements:
# - env TARGET: path to target work dir
##

git clone --no-checkout https://github.com/DaveGamble/cJSON.git \
    "$TARGET/repo"
git -C "$TARGET/repo" checkout 12c4bf1986c288950a3d06da757109a6aa1ece38
