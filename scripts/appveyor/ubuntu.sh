#!/bin/bash

set -xe

sudo apt install -y libboost-filesystem-dev \
                    libboost-iostreams-dev \
                    libboost-program-options-dev \
                    libboost-system-dev \
                    libgit2-dev \
                    libsource-highlight-dev \
                    libsqlite3-dev \
                    libtntnet-dev \
                    ccache

make -j4
make -j4 check
