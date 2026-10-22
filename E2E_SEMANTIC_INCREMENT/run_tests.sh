#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
bash build.sh
./build/test-ast > build/escaping.dot
python3 tests/run_tests.py

./build/test-semantic
python3 tests/test_semantic_sources.py
