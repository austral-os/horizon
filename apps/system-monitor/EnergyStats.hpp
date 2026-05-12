#pragma once

#include "ProcessManager.hpp"
#include <horizon/ChartArea.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    class EnergyStats : public Widget
    {
    public:
        EnergyStats();
        ~EnergyStats() = default;

        void update(const EnergyUsage &usage);

    private:
        std::unique_ptr<Widget> create_stat_row(const std::string &name, Label **value_label_out);

        ChartArea *m_chart{nullptr};

        // Left Panel
        Label *m_lbl_remaining{nullptr};
        Label *m_lbl_time_full{nullptr};
        Label *m_lbl_time_ac{nullptr};

        // Right Panel
        Label *m_lbl_time_empty{nullptr};
        Label *m_lbl_cycles{nullptr};
        Label *m_lbl_health{nullptr};

        std::vector<double> m_history;
        size_t m_max_history{60};
    };
} // namespace horizon
