#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>

namespace horizon::preferences
{
    /**
     * @class WayfireAdapter
     * @brief Display adapter for Wayfire.
     */
    class WayfireAdapter : public IDisplayAdapter
    {
    public:
        WayfireAdapter() = default;
        ~WayfireAdapter() override = default;

        void apply_configs(const std::vector<MonitorConfig>& configs) override;
    };
}
