#pragma once

#include "types.hpp"
#include <vector>

/// Estimate GBM drift (mu) and volatility (sigma) from a price series.
/// Uses log-returns: r_t = ln(S_t / S_{t-1})
///   mu_daily    = mean(r)
///   sigma_daily = std(r)
/// Then annualise: mu = mu_daily*252 + 0.5*sigma_daily^2*252
///                 sigma = sigma_daily * sqrt(252)
struct GBMEstimator {
    static void estimate(
        const std::vector<Price>& prices,
        double& mu_out,
        double& sigma_out
    );

    /// Convenience: fill SimConfig from a price series
    static void fill_config(
        const std::vector<Price>& prices,
        SimConfig& cfg
    );
};
