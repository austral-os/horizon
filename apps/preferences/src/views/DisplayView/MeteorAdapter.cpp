#include <views/DisplayView/MeteorAdapter.hpp>
#include <horizon/Logger.hpp>
#include <cstdlib>

namespace horizon::preferences
{
    void MeteorAdapter::apply_configs(const std::vector<MonitorConfig>& configs)
    {
        LOG_INFO << "[ADAPTER] Applying configs for Meteor (persistence handled by DisplayView)";
    }
}
