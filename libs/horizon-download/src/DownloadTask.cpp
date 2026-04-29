#include "horizon/download/DownloadTask.hpp"
#include "horizon/Logger.hpp"
#include <libsoup/soup.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

namespace horizon {
namespace download {

struct DownloadTask::Impl {
    DownloadTask* parent;
    SoupSession* session = nullptr;
    SoupMessage* message = nullptr;
    GCancellable* cancellable = nullptr;
    std::thread thread;
    
    void run_download() {
        GError* error = nullptr;
        
        // Ensure directory exists
        std::filesystem::path dest_path(parent->m_destination);
        std::filesystem::create_directories(dest_path.parent_path());

        parent->m_impl->message = soup_message_new("GET", parent->m_url.c_str());
        if (!parent->m_impl->message) {
            parent->m_state = DownloadState::FAILED;
            parent->m_error_message = "Invalid URL";
            parent->when_state_changed.run(parent->m_state);
            return;
        }

        std::ofstream file_stream(parent->m_destination, std::ios::binary | std::ios::out);
        if (!file_stream.is_open()) {
            parent->m_state = DownloadState::FAILED;
            parent->m_error_message = "Could not open destination file";
            parent->when_state_changed.run(parent->m_state);
            return;
        }

        GInputStream* input_stream = soup_session_send(session, message, cancellable, &error);
        if (error) {
            if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                parent->m_state = DownloadState::FAILED;
                parent->m_error_message = error->message;
                parent->when_state_changed.run(parent->m_state);
            }
            g_error_free(error);
            return;
        }

        // Get content length
        auto* response_headers = soup_message_get_response_headers(message);
        const char* length_str = soup_message_headers_get_one(response_headers, "Content-Length");
        if (length_str) {
            parent->m_progress.total_bytes = std::stoull(length_str);
        }

        auto start_time = std::chrono::steady_clock::now();
        uint8_t buffer[16384];
        
        while (parent->m_state == DownloadState::DOWNLOADING) {
            gssize bytes_read = g_input_stream_read(input_stream, buffer, sizeof(buffer), cancellable, &error);
            
            if (error) {
                if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
                    g_error_free(error);
                } else {
                    parent->m_state = DownloadState::FAILED;
                    parent->m_error_message = error->message;
                    g_error_free(error);
                    parent->when_state_changed.run(parent->m_state);
                }
                break;
            }

            if (bytes_read <= 0) break;

            file_stream.write(reinterpret_cast<const char*>(buffer), bytes_read);
            parent->m_progress.downloaded_bytes += bytes_read;
            
            // Update stats
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            if (duration > 0) {
                parent->m_progress.speed = (double)parent->m_progress.downloaded_bytes / (duration / 1000.0);
                if (parent->m_progress.total_bytes > 0) {
                    parent->m_progress.progress = (double)parent->m_progress.downloaded_bytes / parent->m_progress.total_bytes;
                    size_t remaining = parent->m_progress.total_bytes - parent->m_progress.downloaded_bytes;
                    if (parent->m_progress.speed > 0)
                        parent->m_progress.eta_seconds = (uint64_t)(remaining / parent->m_progress.speed);
                }
            }
            
            parent->when_progress_changed.run(parent->m_progress);
        }

        g_input_stream_close(input_stream, nullptr, nullptr);
        g_object_unref(input_stream);
        file_stream.close();

        if (parent->m_state == DownloadState::DOWNLOADING) {
            parent->m_state = DownloadState::COMPLETED;
            parent->m_progress.progress = 1.0;
            parent->when_state_changed.run(parent->m_state);
        }
    }
};

DownloadTask::DownloadTask(const std::string& url, const std::string& destination)
    : m_url(url), m_destination(destination), m_impl(std::make_unique<Impl>()) {
    
    m_impl->parent = this;
    m_impl->session = soup_session_new();
    m_impl->cancellable = g_cancellable_new();
}

DownloadTask::~DownloadTask() {
    cancel();
    if (m_impl->thread.joinable()) m_impl->thread.join();
    if (m_impl->session) g_object_unref(m_impl->session);
    if (m_impl->cancellable) g_object_unref(m_impl->cancellable);
    if (m_impl->message) g_object_unref(m_impl->message);
}

void DownloadTask::start() {
    if (m_state == DownloadState::DOWNLOADING) return;

    LOG_INFO << "[DOWNLOAD] Starting: " << m_url << " -> " << m_destination;
    
    m_state = DownloadState::DOWNLOADING;
    when_state_changed.run(m_state);

    if (m_impl->thread.joinable()) m_impl->thread.join();
    m_impl->thread = std::thread(&Impl::run_download, m_impl.get());
}

void DownloadTask::pause() {
    if (m_state == DownloadState::DOWNLOADING) {
        g_cancellable_cancel(m_impl->cancellable);
        m_state = DownloadState::PAUSED;
        when_state_changed.run(m_state);
        // Reset cancellable for next resume
        g_object_unref(m_impl->cancellable);
        m_impl->cancellable = g_cancellable_new();
    }
}

void DownloadTask::resume() {
    if (m_state == DownloadState::PAUSED || m_state == DownloadState::FAILED || m_state == DownloadState::CANCELLED) {
        start();
    }
}

void DownloadTask::cancel() {
    g_cancellable_cancel(m_impl->cancellable);
    m_state = DownloadState::CANCELLED;
    when_state_changed.run(m_state);
}

void DownloadTask::cleanup() {
    // Legacy cleanup call, now handled inside run_download or destructor
}

std::string DownloadTask::filename() const {
    return std::filesystem::path(m_destination).filename().string();
}

} // namespace download
} // namespace horizon
