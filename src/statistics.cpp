#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

double Statistics::percentile(std::vector<Price> sorted, double p) {
    if (sorted.empty()) return 0.0;
    std::sort(sorted.begin(), sorted.end());
    double idx = p * static_cast<double>(sorted.size() - 1);
    std::size_t lo = static_cast<std::size_t>(idx);
    double frac    = idx - static_cast<double>(lo);
    if (lo + 1 < sorted.size())
        return sorted[lo] * (1.0 - frac) + sorted[lo + 1] * frac;
    return sorted[lo];
}

SimStats Statistics::compute(
    const std::vector<Price>& terminal,
    double                    S0,
    const std::string&        backend,
    double                    elapsed_sec
) {
    if (terminal.empty())
        throw std::runtime_error("No terminal prices to compute statistics.");

    const std::size_t N = terminal.size();

    // Mean
    const double mean = std::accumulate(terminal.begin(), terminal.end(), 0.0) / static_cast<double>(N);

    // Std dev
    double var = 0.0;
    for (Price p : terminal)
        var += (p - mean) * (p - mean);
    var /= static_cast<double>(N - 1);

    // Probability of profit
    std::size_t above = 0;
    for (Price p : terminal)
        if (p > S0) ++above;

    SimStats s;
    s.mean        = mean;
    s.std_dev     = std::sqrt(var);
    s.p5          = percentile(terminal, 0.05);
    s.p25         = percentile(terminal, 0.25);
    s.median      = percentile(terminal, 0.50);
    s.p75         = percentile(terminal, 0.75);
    s.p95         = percentile(terminal, 0.95);
    s.min_price   = *std::min_element(terminal.begin(), terminal.end());
    s.max_price   = *std::max_element(terminal.begin(), terminal.end());
    s.prob_profit = static_cast<double>(above) / static_cast<double>(N);
    s.elapsed_sec = elapsed_sec;
    s.backend     = backend;
    return s;
}
