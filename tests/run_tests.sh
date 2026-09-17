#!/usr/bin/env bash
# Run from any directory; build failures and test failures propagate to make.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
make
python3 tests/test_compiler.py "${1:-./lara}"
