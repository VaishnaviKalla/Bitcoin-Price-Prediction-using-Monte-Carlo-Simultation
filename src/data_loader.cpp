#include "data_loader.hpp"
#include "logger.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

DataLoader::DataLoader(const std::string& filepath) {
    load(filepath);
    LOG_INFO("Loaded " << prices_.size() << " price records from " << filepath);
}

void DataLoader::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open data file: " + filepath);

    std::string line;
    bool header_skipped = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Skip header row (any row that starts with a non-digit / letter)
        if (!header_skipped) {
            bool first_char_digit = !line.empty() && (std::isdigit(line[0]) || line[0] == '-');
            if (!first_char_digit) {
                header_skipped = true;
                continue;
            }
            header_skipped = true;
        }

        // Parse CSV: date,close  or  close  (single column)
        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ','))
            fields.push_back(token);

        try {
            if (fields.size() >= 2) {
                // date,close  — take last field as price
                prices_.push_back(std::stod(fields.back()));
            } else if (fields.size() == 1) {
                prices_.push_back(std::stod(fields[0]));
            }
        } catch (...) {
            // Skip malformed lines silently
        }
    }

    if (prices_.empty())
        throw std::runtime_error("No price data found in: " + filepath);
}
