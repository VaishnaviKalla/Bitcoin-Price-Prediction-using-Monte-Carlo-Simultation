#pragma once

#include "types.hpp"

#ifdef USE_CUDA

/// GPU-based Monte Carlo simulator using CUDA + cuRAND.
/// Each CUDA thread simulates one full GBM path.
class SimulatorGPU {
public:
    explicit SimulatorGPU(const SimConfig& cfg);
    ~SimulatorGPU();

    /// Run simulation; returns flat path matrix
    PathMatrix run();

    /// Check if a CUDA-capable device is available
    static bool cuda_available();

private:
    SimConfig cfg_;
    float*    d_paths_  = nullptr; ///< Device path buffer
    void*     d_states_ = nullptr; ///< cuRAND generator states

    void allocate_device();
    void free_device();
};

#endif // USE_CUDA
