#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace horizon {

/**
 * @class FileWatcher
 * @brief Base class for components that need to monitor a file for changes.
 * 
 * Subclasses must implement on_file_changed() to react to file events,
 * and post_watcher_task() to ensure the reaction happens on the main thread.
 */
class FileWatcher {
public:
    FileWatcher() = default;
    virtual ~FileWatcher();

    /**
     * @brief Starts watching the specified file path.
     * @param path The absolute path to the file.
     */
    void start_watching(const std::string& path);

    /**
     * @brief Stops the file watcher and joins the monitoring thread.
     */
    void stop_watching();

protected:
    /**
     * @brief Hook called on the main thread when a file change is detected.
     */
    virtual void on_file_changed() = 0;

    /**
     * @brief Mechanism to schedule a task on the main thread of the application.
     * @param task The function to execute.
     */
    virtual void post_watcher_task(std::function<void()> task) = 0;

private:
    void watch_loop();

    int m_inotify_fd = -1;
    int m_watch_fd = -1;
    std::thread m_watcher_thread;
    std::atomic<bool> m_watcher_running{false};
    std::string m_watched_path;
};

} // namespace horizon
