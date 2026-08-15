#!/bin/bash

apt-get update && \
    apt-get install -y git make libtool

# newer cmake version
wget -q https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-linux-x86_64.sh \
        -O /tmp/cmake-install.sh \
        && chmod +x /tmp/cmake-install.sh \
        && /tmp/cmake-install.sh --skip-license --prefix=/usr/local \
        && rm /tmp/cmake-install.sh
