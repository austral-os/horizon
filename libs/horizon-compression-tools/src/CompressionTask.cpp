#include "horizon/compression/CompressionTask.hpp"

namespace horizon::compression
{
    CompressionTask::CompressionTask() = default;

    CompressionTask::~CompressionTask()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void CompressionTask::start()
    {
        if (m_running) return;
        m_running = true;
        m_cancelled = false;
        m_thread = std::thread([this]() {
            this->run();
            m_running = false;
        });
    }

    void CompressionTask::cancel()
    {
        m_cancelled = true;
    }

    bool CompressionTask::is_running() const
    {
        return m_running;
    }

    void CompressionTask::report_progress(double progress, const std::string& file, const std::string& msg)
    {
        CompressionProgressEvent ev;
        ev.sender = this;
        ev.progress = progress;
        ev.current_file = file;
        ev.status_message = msg;
        when_progress.run(ev);
    }

    void CompressionTask::report_finished(bool success, const std::string& error, const std::string& out)
    {
        CompressionFinishedEvent ev;
        ev.sender = this;
        ev.success = success;
        ev.error_message = error;
        ev.output_path = out;
        when_finished.run(ev);
    }
} // namespace horizon::compression
