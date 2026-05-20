/**
 * simulator_gpu.cu
 *
 * Each CUDA thread simulates one complete GBM path.
 * Random numbers are generated on-device using cuRAND (xorwow).
 *
 * Memory layout: row-major, paths x (steps+1)
 *   d_paths[p * (steps+1) + t] = price of path p at step t
 */

#ifdef USE_CUDA

#include "simulator_gpu.hpp"
#include "logger.hpp"

#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cmath>
#include <stdexcept>


// Helpers
#define CUDA_CHECK(call)                                                      \
    do {                                                                      \
        cudaError_t err = (call);                                             \
        if (err != cudaSuccess) {                                             \
            throw std::runtime_error(                                         \
                std::string("CUDA error: ") + cudaGetErrorString(err) +      \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));         \
        }                                                                     \
    } while(0)


// Kernel: initialise cuRAND states (one per thread)

__global__ void init_rng_kernel(
    curandState* states,
    unsigned long long seed,
    unsigned int num_paths
) {
    unsigned int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_paths) return;
    // Unique sequence per thread via offset
    curand_init(seed, idx, 0, &states[idx]);
}

// Kernel: simulate one GBM path per thread

__global__ void gbm_kernel(
    float*       d_paths,    // (num_paths) x (num_steps+1), row-major
    curandState* d_states,
    unsigned int num_paths,
    unsigned int num_steps,
    float        S0,
    float        drift,      // (mu - 0.5*sigma^2)*dt
    float        diffus      // sigma*sqrt(dt)
) {
    unsigned int p = blockIdx.x * blockDim.x + threadIdx.x;
    if (p >= num_paths) return;

    curandState local_state = d_states[p];
    float S = S0;

    // Step 0 — starting price
    d_paths[p * (num_steps + 1)] = S;

    for (unsigned int t = 0; t < num_steps; ++t) {
        float Z = curand_normal(&local_state);
        S = S * expf(drift + diffus * Z);
        d_paths[p * (num_steps + 1) + (t + 1)] = S;
    }

    d_states[p] = local_state;
}


// Class implementation

SimulatorGPU::SimulatorGPU(const SimConfig& cfg) : cfg_(cfg) {
    allocate_device();
}

SimulatorGPU::~SimulatorGPU() {
    free_device();
}

bool SimulatorGPU::cuda_available() {
    int count = 0;
    cudaGetDeviceCount(&count);
    return count > 0;
}

void SimulatorGPU::allocate_device() {
    const size_t path_bytes  = (size_t)cfg_.num_paths * (cfg_.num_steps + 1) * sizeof(float);
    const size_t state_bytes = (size_t)cfg_.num_paths * sizeof(curandState);

    CUDA_CHECK(cudaMalloc(&d_paths_,  path_bytes));
    CUDA_CHECK(cudaMalloc(&d_states_, state_bytes));

    LOG_DEBUG("GPU allocated " << (path_bytes + state_bytes) / (1024*1024) << " MB");
}

void SimulatorGPU::free_device() {
    if (d_paths_ ) { cudaFree(d_paths_ ); d_paths_  = nullptr; }
    if (d_states_) { cudaFree(d_states_); d_states_ = nullptr; }
}

PathMatrix SimulatorGPU::run() {
    const uint32_t P    = cfg_.num_paths;
    const uint32_t T    = cfg_.num_steps;
    const float    S0   = static_cast<float>(cfg_.S0);
    const float    mu   = static_cast<float>(cfg_.mu);
    const float    sig  = static_cast<float>(cfg_.sigma);
    const float    dt   = static_cast<float>(cfg_.dt);

    const float drift  = (mu - 0.5f * sig * sig) * dt;
    const float diffus = sig * sqrtf(dt);

    // Print device info
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    LOG_INFO("GPU: " << prop.name << "  SMs: " << prop.multiProcessorCount);

    // Kernel launch config
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (static_cast<int>(P) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Seed RNG states
    init_rng_kernel<<<num_blocks, BLOCK_SIZE>>>(
        static_cast<curandState*>(d_states_),
        42ULL,
        P
    );
    CUDA_CHECK(cudaGetLastError());

    // Simulate paths
    gbm_kernel<<<num_blocks, BLOCK_SIZE>>>(
        d_paths_,
        static_cast<curandState*>(d_states_),
        P, T, S0, drift, diffus
    );
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Copy results back to host
    const size_t path_bytes = (size_t)P * (T + 1) * sizeof(float);
    std::vector<float> h_flat(P * (T + 1));
    CUDA_CHECK(cudaMemcpy(h_flat.data(), d_paths_, path_bytes, cudaMemcpyDeviceToHost));

    // Convert flat float buffer → PathMatrix<double>
    PathMatrix paths(P, std::vector<Price>(T + 1));
    for (uint32_t p = 0; p < P; ++p) {
        for (uint32_t t = 0; t <= T; ++t) {
            paths[p][t] = static_cast<double>(h_flat[p * (T + 1) + t]);
        }
    }

    LOG_DEBUG("GPU simulation complete (" << P << " paths x " << T << " steps)");
    return paths;
}

#endif // USE_CUDA
