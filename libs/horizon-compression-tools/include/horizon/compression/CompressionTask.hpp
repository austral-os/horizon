#pragma once

#include "CompressionEvents.hpp"
#include <memory>
#include <thread>
#include <atomic>

namespace horizon::compression
{
    class CompressionTask
    {
    public:
        CompressionTask();
        ~CompressionTask();

        // Horizon Standard Signals
        EventsManager<CompressionProgressEvent> when_progress;
        EventsManager<CompressionFinishedEvent> when_finished;

        void start();
        void cancel();
        bool is_running() const;

    protected:
        // Internal methods for the implementation to call
        void report_progress(double progress, const std::string& file = "", const std::string& msg = "");
        void report_finished(bool success, const std::string& error = "", const std::string& out = "");

        virtual void run() = 0;

        std::atomic<bool> m_running{false};
        std::atomic<bool> m_cancelled{false};
        std::thread m_thread;
    };
} // namespace horizon::compression
