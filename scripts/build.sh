#!/usr/bin/env bash
# scripts/build.sh — configure, build, and optionally run the simulation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# ─── Options (override via env vars) ─────────────────────────────
USE_CUDA="${USE_CUDA:-ON}"
USE_MPI="${USE_MPI:-ON}"
USE_OPENMP="${USE_OPENMP:-ON}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
RUN_AFTER_BUILD="${RUN_AFTER_BUILD:-0}"

echo "=== BTC Monte Carlo — Build Script ==="
echo "  CUDA   : $USE_CUDA"
echo "  MPI    : $USE_MPI"
echo "  OpenMP : $USE_OPENMP"
echo "  Type   : $BUILD_TYPE"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DUSE_CUDA="$USE_CUDA" \
    -DUSE_MPI="$USE_MPI" \
    -DUSE_OPENMP="$USE_OPENMP"

cmake --build . --parallel "$(nproc)"

echo ""
echo "✓ Build complete: $BUILD_DIR/btc_sim"

if [[ "$RUN_AFTER_BUILD" == "1" ]]; then
    echo ""
    echo "=== Running quick test (1000 paths, 365 steps) ==="
    cd "$PROJECT_DIR"
    if [[ "$USE_MPI" == "ON" ]]; then
        mpirun -np 2 "$BUILD_DIR/btc_sim" --paths 1000 --steps 365 --loglevel info
    else
        "$BUILD_DIR/btc_sim" --paths 1000 --steps 365 --loglevel info
    fi
fi
