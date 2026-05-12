#pragma once

#include "ProcessManager.hpp"
#include <horizon/ChartArea.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    class NetworkStats : public Widget
    {
    public:
        NetworkStats();
        ~NetworkStats() = default;

        void update(const NetworkUsage &usage);

    private:
        std::unique_ptr<Widget> create_stat_row(const std::string &name, Label **value_label_out);

        ChartArea *m_chart{nullptr};

        // Left Panel
        Label *m_lbl_rx_packets{nullptr};
        Label *m_lbl_tx_packets{nullptr};
        Label *m_lbl_rx_packets_sec{nullptr};
        Label *m_lbl_tx_packets_sec{nullptr};

        // Right Panel
        Label *m_lbl_rx_total{nullptr};
        Label *m_lbl_tx_total{nullptr};
        Label *m_lbl_rx_speed{nullptr};
        Label *m_lbl_tx_speed{nullptr};

        std::vector<double> m_rx_history;
        std::vector<double> m_tx_history;
        size_t m_max_history{60};
    };
} // namespace horizon
