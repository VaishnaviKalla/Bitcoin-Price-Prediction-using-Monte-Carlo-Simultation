#pragma once

#include <string>
#include <sstream>

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

/// Thread-safe (mutex-protected) logger with level filtering.
class Logger {
public:
    static Logger& instance();

    void set_level(const std::string& level);
    void set_mpi_rank(int rank, int size);

    void debug(const std::string& msg) const;
    void info (const std::string& msg) const;
    void warn (const std::string& msg) const;
    void error(const std::string& msg) const;

private:
    Logger() = default;
    void log(LogLevel level, const std::string& msg) const;

    LogLevel min_level_ = LogLevel::INFO;
    int      mpi_rank_  = 0;
    int      mpi_size_  = 1;
};

// Convenience macro — builds message via stream
#define LOG_INFO(msg)  do { std::ostringstream _s; _s << msg; Logger::instance().info (_s.str()); } while(0)
#define LOG_DEBUG(msg) do { std::ostringstream _s; _s << msg; Logger::instance().debug(_s.str()); } while(0)
#define LOG_WARN(msg)  do { std::ostringstream _s; _s << msg; Logger::instance().warn (_s.str()); } while(0)
#define LOG_ERROR(msg) do { std::ostringstream _s; _s << msg; Logger::instance().error(_s.str()); } while(0)
