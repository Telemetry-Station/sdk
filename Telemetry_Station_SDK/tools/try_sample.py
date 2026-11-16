#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build and run one simulated sensor message. No hardware or account required."""
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    cmake = shutil.which('cmake')
    if not cmake:
        print('Setup needed: install CMake and C development tools for your computer, then reopen the terminal and run this command again.', file=sys.stderr)
        return 1
    print('Try BEAR: one simulated sensor reading. No account or airtime used.', flush=True)
    with tempfile.TemporaryDirectory(prefix='bear-sample-') as directory:
        build = Path(directory)
        commands = [
            ('Preparing the sample', [cmake, '-S', str(ROOT), '-B', str(build)]),
            ('Building the sample', [cmake, '--build', str(build), '--config', 'Debug', '--target', 'simulated_demo']),
        ]
        for label, command in commands:
            print(label + '...', flush=True)
            result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if result.returncode:
                print(result.stdout[-4000:], file=sys.stderr)
                print('Setup did not finish. Check the compiler/build error above. See Telemetry_Station_SDK/docs/GETTING_STARTED.md, step 2.', file=sys.stderr)
                return 1
        executable = next((p for p in [build / 'simulated_demo', build / 'simulated_demo.exe', build / 'Debug/simulated_demo.exe', build / 'Debug/simulated_demo'] if p.is_file()), None)
        if executable is None:
            print('Build completed, but the sample executable was not found. Use the manual build steps in GETTING_STARTED.md.', file=sys.stderr)
            return 1
        return subprocess.run([str(executable)], check=False).returncode


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, KeyboardInterrupt) as error:
        print('Sample stopped: ' + str(error), file=sys.stderr)
        sys.exit(1)
