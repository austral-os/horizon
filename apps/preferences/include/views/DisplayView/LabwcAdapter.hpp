#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>

namespace horizon::preferences
{
    /**
     * @class LabwcAdapter
     * @brief Display adapter for Labwc (wlroots-based).
     */
    class LabwcAdapter : public IDisplayAdapter
    {
    public:
        LabwcAdapter() = default;
        ~LabwcAdapter() override = default;

        void apply_configs(const std::vector<MonitorConfig>& configs) override;
    };
}
