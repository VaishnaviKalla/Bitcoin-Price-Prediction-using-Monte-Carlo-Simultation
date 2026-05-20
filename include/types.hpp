#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>


//  Core types
using Price  = double;
using PathID = uint32_t;

/// All paths: outer = path index, inner = time step
using PathMatrix = std::vector<std::vector<Price>>;


//  Simulation configuration

struct SimConfig {
    // GBM parameters (estimated from data or user-supplied)
    double mu          = 0.0;     ///< Annualised drift
    double sigma       = 0.0;     ///< Annualised volatility
    double dt          = 1.0/365; ///< Time step (1 day)
    double S0          = 0.0;     ///< Starting price

    // Simulation scale
    uint32_t num_paths = 10'000;  ///< Total simulation paths
    uint32_t num_steps = 365;     ///< Steps per path (days)

    // Parallelism
    int  omp_threads   = 0;       ///< 0 = auto-detect
    bool use_gpu       = true;
    bool use_mpi       = true;

    // I/O
    std::string data_file      = "data/btc_prices.csv";
    std::string results_dir    = "results";
    std::string log_level      = "info";   // debug | info | warn | error
    bool save_all_paths        = false;    /// Full path matrix 
    bool save_summary          = true;
};

//  Statistics result
struct SimStats {
    double mean;
    double std_dev;
    double median;
    double p5;          ///< 5th percentile (bear case)
    double p25;
    double p75;
    double p95;         ///< 95th percentile (bull case)
    double min_price;
    double max_price;
    double prob_profit; ///< Fraction of paths ending above S0

    double elapsed_sec; ///< Wall-clock time
    std::string backend; ///< "CUDA" | "OpenMP" | "Serial"
};
