#!/usr/bin/env bash
# scripts/benchmark.sh — run multiple configurations and compare throughput

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(dirname "$SCRIPT_DIR")/build"
BIN="$BUILD_DIR/btc_sim"

if [[ ! -f "$BIN" ]]; then
    echo "Binary not found. Run scripts/build.sh first."
    exit 1
fi

echo "=== Benchmark Suite ==="
echo "Binary: $BIN"
echo ""

PATHS_LIST=(10000 100000 500000)
MPI_PROCS=4

for PATHS in "${PATHS_LIST[@]}"; do
    echo "--- $PATHS paths ---"

    # CPU (single process)
    echo -n "  Serial/OpenMP (1 proc)  : "
    time "$BIN" --paths "$PATHS" --steps 365 --no-gpu --loglevel warn

    # MPI (multi-process)
    if command -v mpirun &>/dev/null; then
        echo -n "  MPI x${MPI_PROCS} + OpenMP       : "
        time mpirun -np "$MPI_PROCS" "$BIN" --paths "$PATHS" --steps 365 --no-gpu --loglevel warn
    fi

    # GPU (if available)
    if command -v nvidia-smi &>/dev/null; then
        echo -n "  CUDA GPU                : "
        time "$BIN" --paths "$PATHS" --steps 365 --loglevel warn
    fi

    echo ""
done

echo "=== Benchmark complete. Results in results/benchmark.csv ==="
