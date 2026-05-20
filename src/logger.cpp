#include "logger.hpp"

#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

static std::mutex g_log_mutex;

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(const std::string& level) {
    if      (level == "debug") min_level_ = LogLevel::DEBUG;
    else if (level == "info" ) min_level_ = LogLevel::INFO;
    else if (level == "warn" ) min_level_ = LogLevel::WARN;
    else if (level == "error") min_level_ = LogLevel::ERROR;
}

void Logger::set_mpi_rank(int rank, int size) {
    mpi_rank_ = rank;
    mpi_size_ = size;
}

void Logger::debug(const std::string& msg) const { log(LogLevel::DEBUG, msg); }
void Logger::info (const std::string& msg) const { log(LogLevel::INFO,  msg); }
void Logger::warn (const std::string& msg) const { log(LogLevel::WARN,  msg); }
void Logger::error(const std::string& msg) const { log(LogLevel::ERROR, msg); }

void Logger::log(LogLevel level, const std::string& msg) const {
    if (level < min_level_) return;

    // Timestamp
    auto now   = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char tbuf[20];
    std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", std::localtime(&t));

    const char* lstr =
        level == LogLevel::DEBUG ? "DEBUG" :
        level == LogLevel::INFO  ? "INFO " :
        level == LogLevel::WARN  ? "WARN " : "ERROR";

    std::lock_guard<std::mutex> lock(g_log_mutex);
    auto& out = (level >= LogLevel::WARN) ? std::cerr : std::cout;
    out << '[' << tbuf << ']';
    if (mpi_size_ > 1)
        out << "[r" << mpi_rank_ << '/' << mpi_size_ << ']';
    out << '[' << lstr << "] " << msg << '\n';
}
