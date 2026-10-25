#!/bin/sh
# SPDX-License-Identifier: MIT
# Runs the kit's logic tests on a computer. No Arduino or modem needed.
set -eu
cd "$(dirname "$0")/.."
BUILD_DIR=$(mktemp -d)
trap 'rm -rf "$BUILD_DIR"' EXIT HUP INT TERM
CC=${CC:-cc}
FLAGS="-std=c99 -Wall -Wextra -Werror -pedantic -I src"

# Arduino compiles .c files as C, so do the same here.
"$CC" $FLAGS -c src/generated/telemetry_payload.c -o "$BUILD_DIR/generated.o"
"$CC" $FLAGS -I src/generated tests/telemetry_payload_example_test.c \
  "$BUILD_DIR/generated.o" -lm -o "$BUILD_DIR/golden_test"
"$BUILD_DIR/golden_test"

"$CC" $FLAGS tests/test_kit.c src/app_payload.c src/ts_mo_tracker.c \
  src/ts_activation.c "$BUILD_DIR/generated.o" -lm -o "$BUILD_DIR/kit_test"
"$BUILD_DIR/kit_test"
