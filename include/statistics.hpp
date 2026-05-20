#pragma once

#include "types.hpp"
#include <vector>

/// Computes descriptive statistics over the terminal prices of all paths.
class Statistics {
public:
    /// terminal: the final price of each simulated path
    static SimStats compute(
        const std::vector<Price>& terminal,
        double                    S0,
        const std::string&        backend,
        double                    elapsed_sec
    );

private:
    static double percentile(std::vector<Price> sorted, double p);
};
