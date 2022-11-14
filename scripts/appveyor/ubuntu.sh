#!/bin/bash

set -xe

scripts/appveyor/ubuntu-prepare.sh

make -j4
make -j4 check
