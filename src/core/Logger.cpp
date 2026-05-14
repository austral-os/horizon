#include "horizon/Logger.hpp"
#include <iostream>
#include <unistd.h>

namespace horizon
{
    Logger &Logger::instance()
    {
        static Logger inst;
        return inst;
    }

    Logger::~Logger()
    {
        if (m_log_file.is_open())
        {
            m_log_file.close();
        }
    }

    void Logger::init(const std::string &app_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_initialized)
            return;

        m_app_id = app_id;
        std::string log_path = "/tmp/" + m_app_id + "_" + std::to_string(getuid()) + ".log";

        // Open and truncate
        m_log_file.open(log_path, std::ios::out | std::ios::trunc);
        if (!m_log_file.is_open())
        {
            std::cerr << "[Logger] Failed to open log file: " << log_path << std::endl;
        }
        else
        {
            m_initialized = true;
        }
    }

    void Logger::log(LogLevel level, const std::string &message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string level_str;
        switch (level)
        {
        case LogLevel::INFO:
            level_str = "[INFO]";
            break;
        case LogLevel::WARNING:
            level_str = "[WARNING]";
            break;
        case LogLevel::ERROR:
            level_str = "[ERROR]";
            break;
        }

        if (m_initialized && m_log_file.is_open())
        {
            m_log_file << level_str << " " << message << std::endl;
        }

        // Also print to stderr for debugging
        std::cerr << level_str << " " << message << std::endl;

        if (m_callback)
        {
            m_callback(level, message);
        }
    }

} // namespace horizon
