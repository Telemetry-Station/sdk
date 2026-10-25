#!/bin/sh
# SPDX-License-Identifier: MIT
# Fails if the driver core links stdio formatting or heap functions.
# Usage: check_core_symbols.sh libtelemetry_station_sdk.a
lib="$1"
core='ts_(bear9704|cmd|setup|mo|mt|jspr|crc|base64)\.c\.o'
bad=$(nm -A "$lib" 2>/dev/null |
      grep -E "($core).* U _*(v?[sf]?n?printf|puts|malloc|calloc|realloc|free)(_chk)?$")
if [ -n "$bad" ]; then
    echo "Driver core uses forbidden functions:"
    echo "$bad"
    exit 1
fi
echo "OK: no stdio formatting or heap use in the driver core"
