#pragma once
#include <horizon/DisplayConfig.hpp>
#include <vector>

namespace horizon::preferences
{
    using MonitorConfig = horizon::MonitorConfig;

    /**
     * @class IDisplayAdapter
     * @brief Interface for applying display settings across different compositors.
     */
    class IDisplayAdapter
    {
    public:
        virtual ~IDisplayAdapter() = default;

        /**
         * @brief Apply the provided monitor configurations to the system.
         * @param configs Vector of monitor configurations.
         */
        virtual void apply_configs(const std::vector<MonitorConfig>& configs) = 0;
    };
}
