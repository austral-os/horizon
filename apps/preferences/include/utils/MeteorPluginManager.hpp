#pragma once
#include <string>

namespace horizon::preferences
{
    class MeteorPluginManager
    {
    public:
        /**
         * @brief Checks if a given plugin is enabled in meteor.ini
         * @param plugin_name The name of the plugin to check
         * @return true if enabled, false otherwise
         */
        static bool is_plugin_enabled(const std::string& plugin_name);

        /**
         * @brief Enables or disables a plugin in meteor.ini
         * @param plugin_name The name of the plugin
         * @param enable true to enable, false to disable
         */
        static void set_plugin_enabled(const std::string& plugin_name, bool enable);
    };
}
