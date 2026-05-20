#include "gbm_params.hpp"
#include "logger.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>

void GBMEstimator::estimate(
    const std::vector<Price>& prices,
    double& mu_out,
    double& sigma_out
) {
    if (prices.size() < 2)
        throw std::runtime_error("Need at least 2 prices to estimate GBM parameters.");

    const std::size_t N = prices.size() - 1;
    std::vector<double> log_returns(N);

    for (std::size_t i = 0; i < N; ++i) {
        if (prices[i] <= 0.0 || prices[i+1] <= 0.0)
            throw std::runtime_error("Non-positive price found in series.");
        log_returns[i] = std::log(prices[i+1] / prices[i]);
    }

    // Daily mean and variance of log-returns
    const double mean = std::accumulate(log_returns.begin(), log_returns.end(), 0.0) / static_cast<double>(N);

    double var = 0.0;
    for (double r : log_returns)
        var += (r - mean) * (r - mean);
    var /= static_cast<double>(N - 1);

    const double sigma_daily = std::sqrt(var);

    // Annualise (252 trading days)
    constexpr double TRADING_DAYS = 252.0;
    sigma_out = sigma_daily * std::sqrt(TRADING_DAYS);
    // Ito correction: mu_annualised = mean_daily*252 + 0.5*sigma_daily^2*252
    mu_out    = mean * TRADING_DAYS + 0.5 * var * TRADING_DAYS;

    LOG_INFO("Estimated mu (annualised)    : " << mu_out);
    LOG_INFO("Estimated sigma (annualised) : " << sigma_out);
}

void GBMEstimator::fill_config(
    const std::vector<Price>& prices,
    SimConfig& cfg
) {
    double mu, sigma;
    estimate(prices, mu, sigma);
    if (cfg.mu    == 0.0) cfg.mu    = mu;
    if (cfg.sigma == 0.0) cfg.sigma = sigma;
    cfg.S0 = prices.back();
}
