#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>

namespace horizon::preferences
{
    /**
     * @class MeteorAdapter
     * @brief Display adapter for Meteor.
     */
    class MeteorAdapter : public IDisplayAdapter
    {
    public:
        MeteorAdapter() = default;
        ~MeteorAdapter() override = default;

        void apply_configs(const std::vector<MonitorConfig>& configs) override;
    };
}
