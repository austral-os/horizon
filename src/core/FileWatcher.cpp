#include <horizon/FileWatcher.hpp>
#include <horizon/Logger.hpp>
#include <sys/inotify.h>
#include <poll.h>
#include <unistd.h>
#include <chrono>

namespace horizon {

FileWatcher::~FileWatcher() {
    stop_watching();
}

void FileWatcher::start_watching(const std::string& path) {
    if (path.empty()) return;
    m_watched_path = path;

    m_inotify_fd = inotify_init();
    if (m_inotify_fd < 0) {
        LOG_ERROR << "[FileWatcher] Failed to initialize inotify";
        return;
    }

    m_watch_fd = inotify_add_watch(m_inotify_fd, m_watched_path.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (m_watch_fd < 0) {
        LOG_ERROR << "[FileWatcher] Failed to add watch for: " << m_watched_path;
        return;
    }

    m_watcher_running = true;
    m_watcher_thread = std::thread(&FileWatcher::watch_loop, this);
    LOG_INFO << "[FileWatcher] Started watching: " << m_watched_path;
}

void FileWatcher::stop_watching() {
    m_watcher_running = false;
    if (m_watcher_thread.joinable()) {
        m_watcher_thread.join();
    }

    if (m_watch_fd >= 0) {
        inotify_rm_watch(m_inotify_fd, m_watch_fd);
        m_watch_fd = -1;
    }

    if (m_inotify_fd >= 0) {
        close(m_inotify_fd);
        m_inotify_fd = -1;
    }
}

void FileWatcher::watch_loop() {
    char buffer[1024];
    struct pollfd pfd = {m_inotify_fd, POLLIN, 0};

    while (m_watcher_running) {
        int ret = poll(&pfd, 1, 500); // 500 ms timeout

        if (ret > 0 && (pfd.revents & POLLIN)) {
            int length = read(m_inotify_fd, buffer, sizeof(buffer));

            if (length > 0) {
                // Debounce to ensure file is fully written
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                
                LOG_INFO << "[FileWatcher] Change detected in: " << m_watched_path;
                post_watcher_task([this]() {
                    this->on_file_changed();
                });
            }
        }
    }
}

} // namespace horizon
