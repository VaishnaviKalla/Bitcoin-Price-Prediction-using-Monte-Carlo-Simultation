#pragma once

#include "types.hpp"
#include <string>
#include <vector>

/// Loads price data from a CSV file.
class DataLoader {
public:
    explicit DataLoader(const std::string& filepath);

    /// Returns prices in chronological order
    const std::vector<Price>& prices() const { return prices_; }

    /// Number of records loaded
    std::size_t size() const { return prices_.size(); }

private:
    std::vector<Price> prices_;
    void load(const std::string& filepath);
};
