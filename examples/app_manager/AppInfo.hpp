#pragma once
#include <string>

namespace app_manager
{
    struct AppInfo
    {
        std::string id;
        std::string name;
        int pid;
        std::string icon;
        bool show_in_dock;
        bool show_in_system_tray;
        bool is_minimized;
    };
} // namespace app_manager
