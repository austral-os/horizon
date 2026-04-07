#include <views/DisplayView/WayfireAdapter.hpp>
#include <horizon/Logger.hpp>
#include <cstdlib>

namespace horizon::preferences
{
    void WayfireAdapter::apply_configs(const std::vector<MonitorConfig>& configs)
    {
        LOG_INFO << "[ADAPTER] Applying configs for Wayfire (persistence handled by DisplayView)";
    }
}
