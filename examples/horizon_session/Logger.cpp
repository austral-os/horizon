#include "Logger.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::~Logger()
{
    if (m_log_file.is_open())
    {
        m_log_file.close();
    }
}

void Logger::init(const std::string &log_file_path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_log_file.is_open())
    {
        m_log_file.close();
    }
    m_log_file.open(log_file_path, std::ios::app);
    m_initialized = m_log_file.is_open();
}

void Logger::log(LogLevel level, const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream time_str;
    time_str << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    std::string level_str;
    switch (level)
    {
    case LogLevel::INFO:
        level_str = "[INFO]";
        break;
    case LogLevel::WARNING:
        level_str = "[WARN]";
        break;
    case LogLevel::ERROR:
        level_str = "[ERRO]";
        break;
    }

    std::string formatted_message = time_str.str() + " " + level_str + " " + message;

    // Remove trailing newline if it already exists (from std::endl) before adding our own newline
    // if necessary
    if (!formatted_message.empty() && formatted_message.back() == '\n')
    {
        formatted_message.pop_back();
    }

    // Output to console
    if (level == LogLevel::ERROR)
    {
        std::cerr << formatted_message << std::endl;
    }
    else
    {
        std::cout << formatted_message << std::endl;
    }

    // Output to file if initialized
    if (m_initialized && m_log_file.is_open())
    {
        m_log_file << formatted_message << std::endl;
        m_log_file.flush();
    }
}
