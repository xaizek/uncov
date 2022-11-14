#!/bin/bash

set -xe

scripts/appveyor/ubuntu-prepare.sh
sudo apt install -y valgrind

make -j4 debug debug/tests/tests

valgrind --fullpath-after=$PWD/ \
         --log-file=valgrind-report \
         --track-origins=yes \
         --track-fds=yes \
         --leak-check=full \
         debug/tests/tests || true

if ! awk '/ERROR SUMMARY:/ { if ($4 != 0) exit 99 }' valgrind-report; then
    cat valgrind-report
    exit 99
fi
