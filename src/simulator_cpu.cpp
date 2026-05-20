#include "simulator_cpu.hpp"
#include "logger.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

#ifdef USE_OPENMP
#include <omp.h>
#endif

SimulatorCPU::SimulatorCPU(const SimConfig& cfg) : cfg_(cfg) {
#ifdef USE_OPENMP
    if (cfg_.omp_threads > 0)
        omp_set_num_threads(cfg_.omp_threads);
    LOG_INFO("OpenMP threads : " << omp_get_max_threads());
#else
    LOG_INFO("OpenMP disabled — serial CPU simulation");
#endif
}

PathMatrix SimulatorCPU::run() {
    const uint32_t P = cfg_.num_paths;
    const uint32_t T = cfg_.num_steps;
    const double   S0    = cfg_.S0;
    const double   mu    = cfg_.mu;
    const double   sigma = cfg_.sigma;
    const double   dt    = cfg_.dt;

    // GBM exact solution per step:
    // S(t+dt) = S(t) * exp((mu - 0.5*sigma^2)*dt + sigma*sqrt(dt)*Z)
    const double drift  = (mu - 0.5 * sigma * sigma) * dt;
    const double diffus = sigma * std::sqrt(dt);

    PathMatrix paths(P, std::vector<Price>(T + 1));

    // Each thread gets its own seeded RNG — avoids contention
#ifdef USE_OPENMP
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        std::mt19937_64 rng(std::random_device{}() ^ (uint64_t(tid) << 32));
        std::normal_distribution<double> norm(0.0, 1.0);

        #pragma omp for schedule(dynamic, 64)
        for (uint32_t p = 0; p < P; ++p) {
            paths[p][0] = S0;
            for (uint32_t t = 0; t < T; ++t) {
                double Z = norm(rng);
                paths[p][t+1] = paths[p][t] * std::exp(drift + diffus * Z);
            }
        }
    }
#else
    std::mt19937_64 rng(std::random_device{}());
    std::normal_distribution<double> norm(0.0, 1.0);
    for (uint32_t p = 0; p < P; ++p) {
        paths[p][0] = S0;
        for (uint32_t t = 0; t < T; ++t) {
            double Z = norm(rng);
            paths[p][t+1] = paths[p][t] * std::exp(drift + diffus * Z);
        }
    }
#endif

    LOG_DEBUG("CPU simulation complete (" << P << " paths x " << T << " steps)");
    return paths;
}
