#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build
cmake --build build --parallel
./build/streampipe --mode sim --window 24 --threshold 3 --interval-ms 200
