#!/bin/bash

set -xe

brew install boost ccache source-highlight libgit2

# retards at Apple symlink `llvm-cov` as `gcov` breaking things
echo "TESTS := '~[new-gcovi-subcommand]'" > config.mk

# at least brew lacks tntnet
make -j4 NO-WEB=y
make -j4 check NO-WEB=y
