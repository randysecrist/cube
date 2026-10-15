#!/bin/sh
set -e
mkdir -p build-aux
autoreconf --install --force --verbose
echo ""
echo "Now run:  ./configure && make"
