#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>
#include <vector>

namespace horizon::preferences
{
    /**
     * @class MeteorConfigWriter
     * @brief Helper for updating Meteor config files from monitor configurations.
     */
    class MeteorConfigWriter
    {
    public:
        /**
         * @brief Updates meteor.ini with the provided monitor settings.
         * @param configs The list of monitor configurations.
         */
        static void update_monitor_config(const std::vector<MonitorConfig>& configs);
    };
}
