#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "horizon/EventsManager.hpp"

namespace horizon {
namespace download {

enum class DownloadState {
    PENDING,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct DownloadProgress {
    double progress; // 0.0 to 1.0
    size_t downloaded_bytes;
    size_t total_bytes;
    double speed; // bytes per second
    uint64_t eta_seconds;
};

class DownloadTask {
public:
    DownloadTask(const std::string& url, const std::string& destination);
    ~DownloadTask();

    // Controls
    void start();
    void pause();
    void resume();
    void cancel();

    // Getters
    std::string url() const { return m_url; }
    std::string destination() const { return m_destination; }
    std::string filename() const;
    DownloadState state() const { return m_state; }
    DownloadProgress progress() const { return m_progress; }
    std::string error_message() const { return m_error_message; }

    // Signals
    EventsManager<DownloadProgress> when_progress_changed;
    EventsManager<DownloadState> when_state_changed;

private:
    std::string m_url;
    std::string m_destination;
    DownloadState m_state = DownloadState::PENDING;
    DownloadProgress m_progress = {0.0, 0, 0, 0.0, 0};
    std::string m_error_message;

    // Internal implementation (PIMPL for libsoup details)
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    void cleanup();

    friend class DownloadManager;
};

} // namespace download
} // namespace horizon
