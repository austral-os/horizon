#pragma once

#include "ProcessManager.hpp"
#include <horizon/ChartArea.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    class MemoryStats : public Widget
    {
    public:
        MemoryStats();
        ~MemoryStats() = default;

        void update(const MemoryUsage &usage);

    private:
        std::unique_ptr<Widget> create_stat_row(const std::string &name, Label **value_label_out);

        ChartArea *m_chart{nullptr};

        // Left Panel
        Label *m_lbl_total_phys{nullptr};
        Label *m_lbl_used_phys{nullptr};
        Label *m_lbl_total_virt{nullptr};
        Label *m_lbl_used_swap{nullptr};

        // Right Panel
        Label *m_lbl_available{nullptr};
        Label *m_lbl_cached{nullptr};
        Label *m_lbl_free{nullptr};
        Label *m_lbl_used_pct{nullptr};

        std::vector<double> m_history;
        size_t m_max_history{60};
    };
} // namespace horizon
