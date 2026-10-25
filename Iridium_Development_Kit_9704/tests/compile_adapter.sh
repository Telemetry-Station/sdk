#!/bin/sh
# SPDX-License-Identifier: MIT
# Checks the Arduino code compiles against a stand-in for Iridium's library.
set -eu
cd "$(dirname "$0")/.."
CXX=${CXX:-c++}
"$CXX" -std=c++11 -Wall -Wextra -Werror -pedantic -fsyntax-only \
    -I tests/mock_vendor -I src src/ts_launchpad.cpp
printf '%s\n' 'PASS: Arduino adapter compiles against a stub of the official 9704 API'
