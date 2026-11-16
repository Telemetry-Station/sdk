#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check configuration boundaries with the actual host C compiler."""
import subprocess
import sys
from pathlib import Path

INCLUDE = Path(__file__).resolve().parents[1] / 'include'


def check(compiler, definitions, accepted):
    result = subprocess.run(
        [compiler, '-std=c99', '-fsyntax-only', '-x', 'c', '-I', str(INCLUDE),
         *['-D' + value for value in definitions], '-'],
        input='#include "ts_config.h"\nint main(void) { return 0; }\n',
        text=True, capture_output=True)
    if (result.returncode == 0) != accepted:
        raise AssertionError(f'{definitions}: expected {"accept" if accepted else "reject"}\n{result.stderr}')


def main(compiler):
    for definitions in [[], ['TS_POLL_BYTE_BUDGET=1'], ['TS_POLL_BYTE_BUDGET=65535'],
                        ['TS_QUERY_IMEI_ON_READY=0'], ['TS_QUERY_IMEI_ON_READY=1']]:
        check(compiler, definitions, True)
    for definition in ['TS_POLL_BYTE_BUDGET=0', 'TS_POLL_BYTE_BUDGET=-1',
                       'TS_POLL_BYTE_BUDGET=65536', 'TS_QUERY_IMEI_ON_READY=2']:
        check(compiler, [definition], False)
    for name in ['CHARGE', 'BOOT', 'SHUTDOWN', 'CMD', 'MO', 'MT']:
        for value in ['0', '2147483648UL']:
            check(compiler, [f'TS_{name}_TIMEOUT_MS={value}'], False)
        check(compiler, [f'TS_{name}_TIMEOUT_MS=2147483647UL'], True)
    print('PASS: portable configuration bounds')


if __name__ == '__main__':
    main(sys.argv[1])
