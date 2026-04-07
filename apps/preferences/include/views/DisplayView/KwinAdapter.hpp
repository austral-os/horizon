#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>

namespace horizon::preferences
{
    /**
     * @class KwinAdapter
     * @brief Display adapter for KWin (KDE).
     */
    class KwinAdapter : public IDisplayAdapter
    {
    public:
        KwinAdapter() = default;
        ~KwinAdapter() override = default;

        void apply_configs(const std::vector<MonitorConfig>& configs) override;
    };
}
