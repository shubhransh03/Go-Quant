"""
Integration test helper that starts the matching_engine binary (if present in build/) and runs the example WebSocket client.

Usage:
    python tools/ws_integration_test.py

This script is intended to be run locally where the binary has been built.
"""
import os
import subprocess
import sys
import time
import signal

BINARY = os.path.join('build', 'matching_engine')
PY_CLIENT = os.path.join('tools', 'example_ws_client.py')

def find_binary():
    # Try Windows exe
    if os.path.exists(BINARY + '.exe'):
        return BINARY + '.exe'
    if os.path.exists(BINARY):
        return BINARY
    return None

def main():
    binary = find_binary()
    proc = None
    if binary:
        print('Found engine binary:', binary)
        proc = subprocess.Popen([binary], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(1.0)  # give server a moment to start
    else:
        print('Engine binary not found; ensure you built the project in build/ and retry.')
        print('You can still run the example client against a running engine.')

    try:
        ret = subprocess.run([sys.executable, PY_CLIENT], check=False)
        print('Client exited with code', ret.returncode)
    finally:
        if proc:
            try:
                proc.terminate()
                proc.wait(timeout=3)
            except Exception:
                proc.kill()

if __name__ == '__main__':
    main()
