#pragma once

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include "DownloadTask.hpp"
#include "horizon/EventsManager.hpp"

namespace horizon {
namespace download {

class DownloadManager {
public:
    static DownloadManager& instance();

    // Create a new download task
    std::shared_ptr<DownloadTask> add_download(const std::string& url, const std::string& destination = "");

    // Task management
    std::vector<std::shared_ptr<DownloadTask>> tasks() const;
    void remove_task(const std::shared_ptr<DownloadTask>& task);
    
    // Global controls
    void pause_all();
    void resume_all();

    // Configuration
    void set_default_download_directory(const std::string& path);
    std::string default_download_directory() const;

    // Signals
    EventsManager<std::shared_ptr<DownloadTask>> when_task_added;
    EventsManager<std::shared_ptr<DownloadTask>> when_task_removed;

private:
    DownloadManager();
    ~DownloadManager();

    std::vector<std::shared_ptr<DownloadTask>> m_tasks;
    mutable std::mutex m_tasks_mutex;
    std::string m_default_dir;

    void load_history();
    void save_history();
};

} // namespace download
} // namespace horizon
