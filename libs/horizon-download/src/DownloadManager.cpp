#include "horizon/download/DownloadManager.hpp"
#include <filesystem>
#include <pwd.h>
#include <unistd.h>

namespace horizon {
namespace download {

DownloadManager& DownloadManager::instance() {
    static DownloadManager manager;
    return manager;
}

DownloadManager::DownloadManager() {
    // Determine default download directory
    const char* home = getenv("HOME");
    if (home) {
        m_default_dir = std::string(home) + "/Downloads";
    } else {
        m_default_dir = "/tmp";
    }
    
    // Create directory if it doesn't exist
    std::filesystem::create_directories(m_default_dir);
    
    load_history();
}

DownloadManager::~DownloadManager() {
    save_history();
}

std::shared_ptr<DownloadTask> DownloadManager::add_download(const std::string& url, const std::string& destination) {
    std::string final_dest = destination;
    if (final_dest.empty()) {
        std::string filename = std::filesystem::path(url).filename().string();
        if (filename.empty() || filename.find('?') != std::string::npos) {
            filename = "download";
        }
        final_dest = m_default_dir + "/" + filename;
    }

    auto task = std::make_shared<DownloadTask>(url, final_dest);
    
    {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        m_tasks.push_back(task);
    }
    
    when_task_added.run(task);
    task->start();
    return task;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::tasks() const {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);
    return m_tasks;
}

void DownloadManager::remove_task(const std::shared_ptr<DownloadTask>& task) {
    {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
            if (*it == task) {
                m_tasks.erase(it);
                break;
            }
        }
    }
    auto task_ptr = task;
    when_task_removed.run(task_ptr);
}

void DownloadManager::pause_all() {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);
    for (auto& task : m_tasks) {
        task->pause();
    }
}

void DownloadManager::resume_all() {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);
    for (auto& task : m_tasks) {
        task->resume();
    }
}

void DownloadManager::set_default_download_directory(const std::string& path) {
    m_default_dir = path;
}

std::string DownloadManager::default_download_directory() const {
    return m_default_dir;
}

void DownloadManager::load_history() {
    // TODO: Implement JSON persistence
}

void DownloadManager::save_history() {
    // TODO: Implement JSON persistence
}

} // namespace download
} // namespace horizon
