#!/bin/sh
# SPDX-License-Identifier: MIT
# Builds the driver with coverage, runs every test, and reports line coverage
# for src/. Fails if any file is below TS_COVERAGE_MIN percent (default 100).
#
#   sh tests/coverage.sh                    # default build
#   sh tests/coverage.sh -DTS_ENABLE_MT=OFF # extra arguments go to CMake
#
# Needs CMake, a C compiler, and gcov (or llvm-cov with Clang).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
build=${TS_COVERAGE_DIR:-/tmp/ts-coverage-build}
min=${TS_COVERAGE_MIN:-100}

rm -rf "$build"
cmake -S "$root" -B "$build" -DTS_COVERAGE=ON "$@" > /dev/null
cmake --build "$build" > /dev/null
ctest --test-dir "$build" --output-on-failure

if grep -qi clang "$build"/CMakeFiles/*/CMakeCCompiler.cmake 2>/dev/null; then
    if command -v xcrun > /dev/null 2>&1; then gcov_cmd="xcrun llvm-cov gcov"
    else gcov_cmd="llvm-cov gcov"; fi
else
    gcov_cmd=${GCOV:-gcov}
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"
for gcda in "$build"/CMakeFiles/telemetry_station_sdk.dir/src/*.gcda; do
    $gcov_cmd -b "$gcda" > /dev/null 2>&1
done

printf '\n%-16s %8s %8s %8s\n' "file" "lines" "missed" "percent"
status=0
for report in ts_*.c.gcov; do
    awk -v name="${report%.gcov}" -v min="$min" '
        BEGIN { FS = ":" }
        $2 + 0 == 0 { next }
        { count = $1; gsub(/ /, "", count) }
        count == "-" { next }
        { total++ }
        count ~ /^(#####|=====)/ { missed++; list = list " " ($2 + 0) }
        END {
            percent = total ? 100 * (total - missed) / total : 100
            printf "%-16s %8d %8d %7.1f%%", name, total, missed, percent
            if (missed) printf "  lines:%s", list
            printf "\n"
            exit (percent + 0 < min + 0)
        }' "$report" || status=1
done

if [ "$status" -ne 0 ]; then
    echo "FAIL: line coverage is below ${min}% in at least one file" >&2
    exit 1
fi
echo "PASS: line coverage is at least ${min}% in every file"
