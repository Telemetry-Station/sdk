#!/bin/sh
# Compatibility entry point. Fresh build directory; never rm -rf user input.
set -eu
exec python3 "$(dirname "$0")/coverage.py" -- "$@"
