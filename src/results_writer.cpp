#include "results_writer.hpp"
#include "logger.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <sys/stat.h>


// Helpers
static std::string timestamp() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H%M%S", std::localtime(&t));
    return buf;
}

// ResultsWriter
ResultsWriter::ResultsWriter(const std::string& output_dir)
    : dir_(output_dir)
{
    ensure_dir();
}

void ResultsWriter::ensure_dir() const {
    // Portable mkdir; ignore EEXIST
    ::mkdir(dir_.c_str(), 0755);
}

void ResultsWriter::write_summary(
    const SimStats& s,
    const SimConfig& cfg
) const {
    const std::string path = dir_ + "/summary_" + timestamp() + ".json";
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write summary to: " + path);

    f << std::fixed << std::setprecision(2);
    f << "{\n";
    f << "  \"backend\"       : \"" << s.backend        << "\",\n";
    f << "  \"num_paths\"     : "   << cfg.num_paths     << ",\n";
    f << "  \"num_steps\"     : "   << cfg.num_steps     << ",\n";
    f << "  \"S0\"            : "   << cfg.S0            << ",\n";
    f << "  \"mu\"            : "   << cfg.mu            << ",\n";
    f << "  \"sigma\"         : "   << cfg.sigma         << ",\n";
    f << "  \"mean\"          : "   << s.mean            << ",\n";
    f << "  \"std_dev\"       : "   << s.std_dev         << ",\n";
    f << "  \"p5\"            : "   << s.p5              << ",\n";
    f << "  \"p25\"           : "   << s.p25             << ",\n";
    f << "  \"median\"        : "   << s.median          << ",\n";
    f << "  \"p75\"           : "   << s.p75             << ",\n";
    f << "  \"p95\"           : "   << s.p95             << ",\n";
    f << "  \"min_price\"     : "   << s.min_price       << ",\n";
    f << "  \"max_price\"     : "   << s.max_price       << ",\n";
    f << "  \"prob_profit\"   : "   << s.prob_profit     << ",\n";
    f << "  \"elapsed_sec\"   : "   << s.elapsed_sec     << "\n";
    f << "}\n";

    LOG_INFO("Summary written : " << path);
}

void ResultsWriter::write_paths(const PathMatrix& paths) const {
    const std::string path = dir_ + "/paths_" + timestamp() + ".csv";
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write paths to: " + path);

    f << std::fixed << std::setprecision(2);
    for (const auto& p : paths) {
        for (std::size_t t = 0; t < p.size(); ++t) {
            if (t) f << ',';
            f << p[t];
        }
        f << '\n';
    }
    LOG_INFO("Paths written   : " << path);
}

void ResultsWriter::append_benchmark(
    const SimStats& s,
    const SimConfig& cfg
) const {
    const std::string path = dir_ + "/benchmark.csv";
    bool write_header = false;
    {
        std::ifstream check(path);
        write_header = !check.good();
    }

    std::ofstream f(path, std::ios::app);
    if (!f) throw std::runtime_error("Cannot write benchmark to: " + path);

    if (write_header)
        f << "timestamp,backend,num_paths,num_steps,elapsed_sec,mean,p5,p95,prob_profit\n";

    f << std::fixed << std::setprecision(4);
    f << timestamp()     << ','
      << s.backend       << ','
      << cfg.num_paths   << ','
      << cfg.num_steps   << ','
      << s.elapsed_sec   << ','
      << s.mean          << ','
      << s.p5            << ','
      << s.p95           << ','
      << s.prob_profit   << '\n';
}
