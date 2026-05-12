#pragma once

#include "ProcessManager.hpp"
#include <horizon/ChartArea.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    class DiskStats : public Widget
    {
    public:
        DiskStats();
        ~DiskStats() = default;

        void update(const DiskUsage &usage);

    private:
        std::unique_ptr<Widget> create_stat_row(const std::string &name, Label **value_label_out);

        ChartArea *m_chart{nullptr};

        // Left Panel
        Label *m_lbl_reads_completed{nullptr};
        Label *m_lbl_writes_completed{nullptr};
        Label *m_lbl_read_speed{nullptr};
        Label *m_lbl_write_speed{nullptr};

        // Right Panel
        Label *m_lbl_total_read{nullptr};
        Label *m_lbl_total_written{nullptr};
        Label *m_lbl_read_speed_r{nullptr};
        Label *m_lbl_write_speed_r{nullptr};

        std::vector<double> m_read_history;
        std::vector<double> m_write_history;
        size_t m_max_history{60};
    };
} // namespace horizon
