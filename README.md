# Bitcoin Price Prediction — Monte Carlo + GBM

> Parallelised simulation using **MPI**, **OpenMP**, and **CUDA** in C++17.

Simulates future Bitcoin prices using **Geometric Brownian Motion (GBM)**,
the canonical stochastic model used in quantitative finance.
GBM parameters (drift μ and volatility σ) are estimated automatically
from historical price data supplied as CSV.

---

## Table of Contents

- [Theory](#theory)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Build](#build)
- [Usage](#usage)
- [Output](#output)
- [Visualisation](#visualisation)
- [Benchmarking](#benchmarking)
- [Project Structure](#project-structure)
- [Contributing](#contributing)

---

## Theory

### Geometric Brownian Motion

A GBM price process satisfies the SDE:

```
dS = μ·S·dt + σ·S·dW
```

where `W` is a Wiener process. The exact discrete solution used per step is:

```
S(t+dt) = S(t) · exp( (μ - σ²/2)·dt + σ·√dt · Z )
```

`Z ~ N(0,1)` is sampled fresh at every step.

### Parameter Estimation

From daily log-returns `r_t = ln(S_t / S_{t-1})`:

| Parameter | Formula |
|-----------|---------|
| σ (annualised) | `std(r) · √252` |
| μ (annualised) | `mean(r)·252 + ½·σ²` (Ito correction) |

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     MPI layer                           │
│         Distributes num_paths across processes          │
│                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │ OpenMP      │  │ OpenMP      │  │ OpenMP      │    │
│  │ (per rank)  │  │ (per rank)  │  │ (per rank)  │    │
│  │             │  │             │  │             │    │
│  │  ┌────────┐ │  │  ┌────────┐ │  │  ┌────────┐ │    │
│  │  │  CUDA  │ │  │  │  CUDA  │ │  │  │  CUDA  │ │    │
│  │  │ kernel │ │  │  │ kernel │ │  │  │ kernel │ │    │
│  │  └────────┘ │  │  └────────┘ │  │  └────────┘ │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────┘
         │                                   │
         └─── MPI_Gather ───────────────────▶ Rank 0
                                              │
                                         Statistics &
                                          JSON output
```

| Layer   | Role |
|---------|------|
| MPI     | Splits total paths across distributed processes (nodes/machines) |
| OpenMP  | Multi-threads the CPU path simulation within each process |
| CUDA    | Each thread simulates one complete GBM path on the GPU |

---

## Prerequisites

| Tool | Minimum version | Notes |
|------|----------------|-------|
| CMake | 3.18 | |
| C++ compiler | GCC 9 / Clang 11 | C++17 required |
| CUDA Toolkit | 11.0 | Optional, disable with `-DUSE_CUDA=OFF` |
| MPI | OpenMPI 4 / MPICH | Optional, disable with `-DUSE_MPI=OFF` |
| Python 3 | 3.9+ | Optional, for visualisation only |

Install on Ubuntu:
```bash
sudo apt-get install build-essential cmake libopenmpi-dev openmpi-bin
# CUDA: follow https://developer.nvidia.com/cuda-downloads
```

---

## Build

```bash
# Full build (CUDA + MPI + OpenMP)
bash scripts/build.sh

# CPU-only build
USE_CUDA=OFF bash scripts/build.sh

# No MPI
USE_MPI=OFF bash scripts/build.sh

# Build and run a quick test
RUN_AFTER_BUILD=1 bash scripts/build.sh
```

Or manually with CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DUSE_CUDA=ON \
         -DUSE_MPI=ON \
         -DUSE_OPENMP=ON
cmake --build . --parallel
```

---

## Usage

```bash
# Basic run (10 000 paths, 365 days)
./build/btc_sim

# Custom parameters
./build/btc_sim --paths 100000 --steps 730 --data data/btc_prices.csv

# With MPI (4 processes)
mpirun -np 4 ./build/btc_sim --paths 500000

# Override GBM parameters manually
./build/btc_sim --mu 0.8 --sigma 0.7 --paths 50000

# Save full path matrix to CSV
./build/btc_sim --save-paths --paths 1000

# Debug logging
./build/btc_sim --loglevel debug
```

### All Options

| Flag | Default | Description |
|------|---------|-------------|
| `--paths N` | 10000 | Total simulation paths |
| `--steps N` | 365 | Steps per path (days) |
| `--threads N` | auto | OpenMP threads per process |
| `--no-gpu` | — | Force CPU-only mode |
| `--data FILE` | `data/btc_prices.csv` | Historical price CSV |
| `--out DIR` | `results` | Output directory |
| `--mu F` | estimated | Annual drift override |
| `--sigma F` | estimated | Annual volatility override |
| `--save-paths` | — | Save full path matrix |
| `--loglevel L` | `info` | `debug\|info\|warn\|error` |

---

## Output

All output is written to `results/` (configurable via `--out`).

| File | Description |
|------|-------------|
| `summary_<ts>.json` | Statistics: mean, std, percentiles, prob_profit, elapsed |
| `paths_<ts>.csv` | Full path matrix (only when `--save-paths` is set) |
| `benchmark.csv` | Appended benchmark row per run |

Example summary:
```json
{
  "backend"       : "CUDA",
  "num_paths"     : 100000,
  "num_steps"     : 365,
  "S0"            : 96720.45,
  "mu"            : 0.87,
  "sigma"         : 0.73,
  "mean"          : 132450.00,
  "std_dev"       : 78320.50,
  "p5"            : 28400.00,
  "median"        : 112200.00,
  "p95"           : 290000.00,
  "prob_profit"   : 0.68,
  "elapsed_sec"   : 0.42
}
```

---

## Visualisation

Requires `matplotlib` and `numpy`:
```bash
pip install matplotlib numpy
```

```bash
# Fan chart + histogram
python scripts/visualise.py --paths results/paths_*.csv

# Benchmark comparison chart
python scripts/visualise.py --benchmark results/benchmark.csv
```

---

## Benchmarking

```bash
bash scripts/benchmark.sh
```

This runs Serial/OpenMP, MPI, and CUDA backends at 10k / 100k / 500k paths
and appends timing to `results/benchmark.csv`.

---

## Project Structure

```
btc_monte_carlo/
├── CMakeLists.txt          # Build system
├── README.md
├── data/
│   └── btc_prices.csv      # Sample historical prices
├── include/
│   ├── types.hpp           # SimConfig, SimStats, PathMatrix
│   ├── data_loader.hpp
│   ├── gbm_params.hpp
│   ├── simulator_cpu.hpp
│   ├── simulator_gpu.hpp   # CUDA guard: #ifdef USE_CUDA
│   ├── statistics.hpp
│   ├── results_writer.hpp
│   └── logger.hpp
├── src/
│   ├── main.cpp            # Orchestration (MPI scatter/gather)
│   ├── data_loader.cpp
│   ├── gbm_params.cpp
│   ├── simulator_cpu.cpp   # OpenMP parallelism
│   ├── simulator_gpu.cu    # CUDA kernel + cuRAND
│   ├── statistics.cpp
│   ├── results_writer.cpp
│   └── logger.cpp
├── results/                # Generated output (gitignored)
└── scripts/
    ├── build.sh
    ├── benchmark.sh
    └── visualise.py
```

---

## Contributing

1. Fork the repo
2. Create a feature branch: `git checkout -b feature/my-change`
3. Run the full build and a quick sanity test
4. Submit a pull request

---

## License

MIT
