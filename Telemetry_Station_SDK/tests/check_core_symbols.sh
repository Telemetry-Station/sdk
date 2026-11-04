#!/bin/sh
# Fail closed on tool/read errors; include all core members, including GNSS.
set -eu
if ! symbols=$(nm -A "$1" 2>&1); then
    printf '%s\n' "$symbols" >&2
    exit 1
fi
if printf '%s\n' "$symbols" | grep -E ' U _*(v?[sf]?n?printf|puts|malloc|calloc|realloc|free)(_chk)?$'; then
    echo 'FAIL: formatting or heap function in SDK core' >&2
    exit 1
fi
echo 'PASS: no formatting or heap functions in SDK core'
