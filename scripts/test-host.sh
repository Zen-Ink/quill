#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

CXX=${CXX:-c++}
mkdir -p build/tests
"$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Isrc tests/test_clipping.cpp -o build/tests/test_clipping
ASAN_OPTIONS=detect_leaks=1 build/tests/test_clipping

echo "host tests passed"
