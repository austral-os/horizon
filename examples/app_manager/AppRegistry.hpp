#pragma once
#include "AppInfo.hpp"
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace app_manager
{
    class AppRegistry
    {
    public:
        void add_app(const AppInfo &info)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_apps[info.id] = info;
            m_cv.notify_all();
        }

        void remove_app(const std::string &app_id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_apps.erase(app_id);
        }

        std::vector<AppInfo> get_apps() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<AppInfo> apps;
            for (const auto &[id, info] : m_apps)
            {
                apps.push_back(info);
            }
            return apps;
        }

        bool wait_for_app(const std::string &app_id, int timeout_ms)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                 [this, app_id] { return m_apps.find(app_id) != m_apps.end(); });
        }

    private:
        std::map<std::string, AppInfo> m_apps;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
    };
} // namespace app_manager
