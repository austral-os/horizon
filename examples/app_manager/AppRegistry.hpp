#pragma once
#include "AppInfo.hpp"
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

    private:
        std::map<std::string, AppInfo> m_apps;
        mutable std::mutex m_mutex;
    };
} // namespace app_manager
