#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
bash build.sh
./build/quectoc-ast "${1:-examples/sample.qc}" > build/ast.dot
dot -Tsvg build/ast.dot -o build/ast.svg
printf '%s\n' 'Created build/ast.dot and build/ast.svg'
