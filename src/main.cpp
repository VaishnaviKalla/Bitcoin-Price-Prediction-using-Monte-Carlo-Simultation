/**
 * btc_monte_carlo — main entry point
 *
 * Parallelism tiers:
 *   1. MPI  — distributes num_paths across processes
 *   2. OpenMP — each process uses multiple CPU threads (CPU path)
 *   3. CUDA   — optional GPU acceleration per process
 *
 * Usage:
 *   mpirun -np 4 ./btc_sim [options]
 *
 * Options:
 *   --paths  N      Total simulation paths         (default 10000)
 *   --steps  N      Steps per path (days)          (default 365)
 *   --threads N     OpenMP threads per process     (default auto)
 *   --no-gpu        Force CPU-only simulation
 *   --data   FILE   CSV price file                 (default data/btc_prices.csv)
 *   --out    DIR    Results output directory       (default results)
 *   --mu     F      Override drift (annualised)
 *   --sigma  F      Override volatility (annualised)
 *   --save-paths    Also save full path matrix to CSV
 *   --loglevel L    debug|info|warn|error           (default info)
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <numeric>

#ifdef USE_MPI
#include <mpi.h>
#endif

#include "types.hpp"
#include "logger.hpp"
#include "data_loader.hpp"
#include "gbm_params.hpp"
#include "simulator_cpu.hpp"
#include "statistics.hpp"
#include "results_writer.hpp"

#ifdef USE_CUDA
#include "simulator_gpu.hpp"
#endif

// CLI parsing (minimal, no external deps)
static SimConfig parse_args(int argc, char** argv) {
    SimConfig cfg;
    for (int i = 1; i < argc; ++i) {
        auto arg = std::string(argv[i]);
        if      (arg == "--paths"      && i+1 < argc) cfg.num_paths     = std::stoul(argv[++i]);
        else if (arg == "--steps"      && i+1 < argc) cfg.num_steps     = std::stoul(argv[++i]);
        else if (arg == "--threads"    && i+1 < argc) cfg.omp_threads   = std::stoi(argv[++i]);
        else if (arg == "--no-gpu"                  ) cfg.use_gpu       = false;
        else if (arg == "--data"       && i+1 < argc) cfg.data_file     = argv[++i];
        else if (arg == "--out"        && i+1 < argc) cfg.results_dir   = argv[++i];
        else if (arg == "--mu"         && i+1 < argc) cfg.mu            = std::stod(argv[++i]);
        else if (arg == "--sigma"      && i+1 < argc) cfg.sigma         = std::stod(argv[++i]);
        else if (arg == "--save-paths"              ) cfg.save_all_paths= true;
        else if (arg == "--loglevel"   && i+1 < argc) cfg.log_level     = argv[++i];
        else if (arg == "--help" || arg == "-h") {
            std::cout <<
                "Usage: btc_sim [options]\n"
                "  --paths  N       Number of simulation paths (default 10000)\n"
                "  --steps  N       Steps per path in days     (default 365)\n"
                "  --threads N      OpenMP threads per process (default auto)\n"
                "  --no-gpu         Force CPU-only mode\n"
                "  --data   FILE    CSV price file             (default data/btc_prices.csv)\n"
                "  --out    DIR     Output directory           (default results)\n"
                "  --mu     F       Override drift  (annualised)\n"
                "  --sigma  F       Override volatility (annualised)\n"
                "  --save-paths     Write full path matrix to CSV\n"
                "  --loglevel L     debug|info|warn|error      (default info)\n";
            std::exit(0);
        }
    }
    return cfg;
}


// Collect terminal prices from a PathMatrix
static std::vector<Price> terminal_prices(const PathMatrix& paths) {
    std::vector<Price> t;
    t.reserve(paths.size());
    for (const auto& p : paths)
        t.push_back(p.back());
    return t;
}


// Main
int main(int argc, char** argv) {

    int rank = 0, world_size = 1;

#ifdef USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
#endif

    SimConfig cfg = parse_args(argc, argv);

    // Logger setup 
    Logger::instance().set_level(cfg.log_level);
    Logger::instance().set_mpi_rank(rank, world_size);

    if (rank == 0) {
        LOG_INFO("Bitcoin Monte Carlo Simulation");
        LOG_INFO("================================");
    }

    //  Load price data (all ranks, small file) 
    DataLoader loader(cfg.data_file);
    if (loader.size() < 2)
        throw std::runtime_error("Need at least 2 price records.");

    // Estimate GBM params from data (rank-0 logs this)
    double mu_est = cfg.mu, sigma_est = cfg.sigma;
    if (mu_est == 0.0 || sigma_est == 0.0)
        GBMEstimator::estimate(loader.prices(), mu_est, sigma_est);

    if (cfg.mu    == 0.0) cfg.mu    = mu_est;
    if (cfg.sigma == 0.0) cfg.sigma = sigma_est;
    cfg.S0 = loader.prices().back();

    if (rank == 0) {
        LOG_INFO("Starting price : $" << static_cast<long>(cfg.S0));
        LOG_INFO("Drift     (mu) : " << cfg.mu   << "  (annualised)");
        LOG_INFO("Volatility(σ)  : " << cfg.sigma << "  (annualised)");
        LOG_INFO("Paths          : " << cfg.num_paths);
        LOG_INFO("Steps (days)   : " << cfg.num_steps);
        LOG_INFO("MPI processes  : " << world_size);
    }

    // Distribute paths across MPI ranks 
    uint32_t local_paths = cfg.num_paths / static_cast<uint32_t>(world_size);
    uint32_t remainder   = cfg.num_paths % static_cast<uint32_t>(world_size);
    if (static_cast<uint32_t>(rank) < remainder) local_paths++;

    SimConfig local_cfg = cfg;
    local_cfg.num_paths = local_paths;

    LOG_DEBUG("Rank " << rank << " simulating " << local_paths << " paths");

    //Select backend 
    auto t0 = std::chrono::high_resolution_clock::now();
    PathMatrix local_paths_matrix;
    std::string backend;

#ifdef USE_CUDA
    bool gpu_ok = cfg.use_gpu && SimulatorGPU::cuda_available();
    if (gpu_ok) {
        LOG_INFO("Backend: CUDA (rank " << rank << ")");
        SimulatorGPU gpu_sim(local_cfg);
        local_paths_matrix = gpu_sim.run();
        backend = "CUDA";
    } else
#endif
    {
        LOG_INFO("Backend: CPU/OpenMP (rank " << rank << ")");
        SimulatorCPU cpu_sim(local_cfg);
        local_paths_matrix = cpu_sim.run();
        backend = "OpenMP";
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    LOG_INFO("Rank " << rank << " finished in " << elapsed << " s");

    // Gather terminal prices on rank 0 
    std::vector<Price> local_terminal = terminal_prices(local_paths_matrix);

#ifdef USE_MPI
    // Gather counts from all ranks
    std::vector<int> counts(world_size), displs(world_size);
    int lc = static_cast<int>(local_terminal.size());
    MPI_Gather(&lc, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<Price> all_terminal;
    if (rank == 0) {
        displs[0] = 0;
        for (int i = 1; i < world_size; ++i)
            displs[i] = displs[i-1] + counts[i-1];
        all_terminal.resize(cfg.num_paths);
    }
    MPI_Gatherv(
        local_terminal.data(), lc, MPI_DOUBLE,
        all_terminal.data(), counts.data(), displs.data(), MPI_DOUBLE,
        0, MPI_COMM_WORLD
    );
#else
    std::vector<Price> all_terminal = local_terminal;
#endif

    // Rank 0: compute stats and write results 
    if (rank == 0) {
        SimStats stats = Statistics::compute(all_terminal, cfg.S0, backend, elapsed);

        LOG_INFO("=== Simulation Results ===");
        LOG_INFO("Mean final price : $" << static_cast<long>(stats.mean));
        LOG_INFO("Std deviation    : $" << static_cast<long>(stats.std_dev));
        LOG_INFO("5th percentile   : $" << static_cast<long>(stats.p5));
        LOG_INFO("Median           : $" << static_cast<long>(stats.median));
        LOG_INFO("95th percentile  : $" << static_cast<long>(stats.p95));
        LOG_INFO("Prob of profit   : " << (stats.prob_profit * 100.0) << "%");

        ResultsWriter writer(cfg.results_dir);
        writer.write_summary(stats, cfg);
        writer.append_benchmark(stats, cfg);
        if (cfg.save_all_paths)
            writer.write_paths(local_paths_matrix); // rank-0 local portion

        LOG_INFO("Results written to: " << cfg.results_dir);
    }

#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 0;
}
