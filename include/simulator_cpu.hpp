#pragma once

#include "types.hpp"

/// CPU-based Monte Carlo simulator.
/// Uses OpenMP to parallelise across paths when USE_OPENMP is defined.
class SimulatorCPU {
public:
    explicit SimulatorCPU(const SimConfig& cfg);

    /// Run simulation; returns flat path matrix (num_paths x num_steps+1)
    PathMatrix run();

private:
    SimConfig cfg_;
};
