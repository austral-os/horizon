#pragma once
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel
{
    INFO,
    WARNING,
    ERROR
};

class Logger
{
public:
    static Logger &instance();

    void init(const std::string &log_file_path);
    void log(LogLevel level, const std::string &message);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::ofstream m_log_file;
    std::mutex m_mutex;
    bool m_initialized = false;
};

class LoggerStream
{
public:
    LoggerStream(LogLevel level) : m_level(level) {}
    ~LoggerStream()
    {
        Logger::instance().log(m_level, m_stream.str());
    }

    template <typename T> LoggerStream &operator<<(const T &value)
    {
        m_stream << value;
        return *this;
    }

    // Support for std::endl and other manipulators
    LoggerStream &operator<<(std::ostream &(*manip)(std::ostream &))
    {
        manip(m_stream);
        return *this;
    }

private:
    LogLevel m_level;
    std::ostringstream m_stream;
};

#define LOG_INFO LoggerStream(LogLevel::INFO)
#define LOG_WARNING LoggerStream(LogLevel::WARNING)
#define LOG_ERROR LoggerStream(LogLevel::ERROR)
