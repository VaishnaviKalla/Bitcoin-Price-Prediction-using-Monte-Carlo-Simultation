#pragma once

#include "types.hpp"
#include <string>

/// Writes simulation results to disk.
class ResultsWriter {
public:
    explicit ResultsWriter(const std::string& output_dir);

    /// Save summary statistics as JSON
    void write_summary(const SimStats& stats, const SimConfig& cfg) const;

    /// Save all simulated paths as CSV (can be large)
    void write_paths(const PathMatrix& paths) const;

    /// Append one-line benchmark record to benchmark.csv
    void append_benchmark(const SimStats& stats, const SimConfig& cfg) const;

private:
    std::string dir_;
    void ensure_dir() const;
};
