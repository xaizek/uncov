#!/bin/bash

set -xe

sudo apt install --quiet --assume-yes --no-install-recommends \
                 libboost-filesystem-dev \
                 libboost-iostreams-dev \
                 libboost-program-options-dev \
                 libboost-system-dev \
                 libgit2-dev \
                 libsource-highlight-dev \
                 libsqlite3-dev \
                 libtntnet-dev \
                 ccache
