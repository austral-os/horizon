#include "NetworkStats.hpp"
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    NetworkStats::NetworkStats() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(15);
        set_margin(20);

        auto container = std::make_unique<AquaObject>();
        container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        container->set_accent_color(WidgetAccentColor::Custom);
        container->set_color1(Color(0.8f, 0.9f, 0.4f, 1.0f));
        container->set_color2(Color(0.8f, 0.8f, 0.4f, 1.0f));
        container->set_border_width(1);
        container->set_corner_radius(15);

        // Left Panel
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_panel->set_fixed_size(300);
        left_panel->set_margin(15);
        left_panel->set_spacing(5);
        left_panel->add_child(create_stat_row("Packets in:", &m_lbl_rx_packets));
        left_panel->add_child(create_stat_row("Packets out:", &m_lbl_tx_packets));
        left_panel->add_child(create_stat_row("Packets in/sec:", &m_lbl_rx_packets_sec));
        left_panel->add_child(create_stat_row("Packets out/sec:", &m_lbl_tx_packets_sec));

        // Center: The Chart
        auto chart_wrapper = std::make_unique<Widget>();
        chart_wrapper->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        chart_wrapper->set_position_type(FILL);

        auto chart = std::make_unique<ChartArea>();
        m_chart = chart.get();
        m_chart->set_smooth_curves(true);
        m_chart->set_show_grid(true);
        m_chart->set_show_legend(true);
        m_chart->set_show_axes(false);
        m_chart->set_auto_scale(true);
        m_chart->set_margins(10, 10, 10, 10);
        m_chart->set_position_type(FILL);
        m_chart->set_bg_color(Color(1.0f, 1.0f, 1.0f, 0.0f));

        chart_wrapper->add_child(std::move(chart));

        // Right Panel
        auto right_panel = std::make_unique<Widget>();
        right_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_panel->set_fixed_size(300);
        right_panel->set_margin(15);
        right_panel->set_spacing(5);
        right_panel->add_child(create_stat_row("Data Received:", &m_lbl_rx_total));
        right_panel->add_child(create_stat_row("Data Sent:", &m_lbl_tx_total));
        right_panel->add_child(create_stat_row("Receive Speed:", &m_lbl_rx_speed));
        right_panel->add_child(create_stat_row("Send Speed:", &m_lbl_tx_speed));

        container->add_child(std::move(left_panel));
        container->add_child(std::move(chart_wrapper));
        container->add_child(std::move(right_panel));

        add_child(std::move(container));

        m_rx_history.resize(m_max_history, 0.0);
        m_tx_history.resize(m_max_history, 0.0);
    }

    std::unique_ptr<Widget> NetworkStats::create_stat_row(const std::string &name,
                                                          Label **value_label_out)
    {
        auto row = std::make_unique<Widget>();
        row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row->set_fixed_size(25);

        auto name_lbl = std::make_unique<Label>(name);
        name_lbl->set_alignment(TextAlignment::Left);
        name_lbl->set_position_type(FILL);
        name_lbl->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        name_lbl->set_text_color(Color(0.2f, 0.3f, 0.1f, 1.0f));
        name_lbl->set_font_size(16);

        auto value_lbl = std::make_unique<Label>("-");
        value_lbl->set_alignment(TextAlignment::Right);
        value_lbl->set_text_color(Color(0.2f, 0.3f, 0.1f, 1.0f));
        value_lbl->set_font_size(16);

        *value_label_out = value_lbl.get();

        row->add_child(std::move(name_lbl));
        row->add_child(std::move(value_lbl));

        return row;
    }

    void NetworkStats::update(const NetworkUsage &usage)
    {
        char buf[64];

        snprintf(buf, sizeof(buf), "%lu", usage.rx_packets);
        if (m_lbl_rx_packets)
            m_lbl_rx_packets->set_text(buf);

        snprintf(buf, sizeof(buf), "%lu", usage.tx_packets);
        if (m_lbl_tx_packets)
            m_lbl_tx_packets->set_text(buf);

        snprintf(buf, sizeof(buf), "%.1f p/s", usage.rx_packets_per_sec);
        if (m_lbl_rx_packets_sec)
            m_lbl_rx_packets_sec->set_text(buf);

        snprintf(buf, sizeof(buf), "%.1f p/s", usage.tx_packets_per_sec);
        if (m_lbl_tx_packets_sec)
            m_lbl_tx_packets_sec->set_text(buf);

        auto format_bytes = [](uint64_t bytes)
        {
            char b[64];
            double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
            if (gb >= 1.0)
            {
                snprintf(b, sizeof(b), "%.2f GB", gb);
            }
            else
            {
                double mb = (double)bytes / (1024.0 * 1024.0);
                snprintf(b, sizeof(b), "%.1f MB", mb);
            }
            return std::string(b);
        };

        if (m_lbl_rx_total)
            m_lbl_rx_total->set_text(format_bytes(usage.rx_bytes));
        if (m_lbl_tx_total)
            m_lbl_tx_total->set_text(format_bytes(usage.tx_bytes));

        auto format_speed = [](double kb_sec)
        {
            char b[64];
            if (kb_sec >= 1024.0)
            {
                snprintf(b, sizeof(b), "%.2f MB/s", kb_sec / 1024.0);
            }
            else
            {
                snprintf(b, sizeof(b), "%.1f KB/s", kb_sec);
            }
            return std::string(b);
        };

        if (m_lbl_rx_speed)
            m_lbl_rx_speed->set_text(format_speed(usage.rx_kb_per_sec));
        if (m_lbl_tx_speed)
            m_lbl_tx_speed->set_text(format_speed(usage.tx_kb_per_sec));

        m_rx_history.push_back(usage.rx_packets_per_sec);
        if (m_rx_history.size() > m_max_history)
            m_rx_history.erase(m_rx_history.begin());

        m_tx_history.push_back(usage.tx_packets_per_sec);
        if (m_tx_history.size() > m_max_history)
            m_tx_history.erase(m_tx_history.begin());

        m_chart->clear_series();
        m_chart->add_series("RX Packets/s", Color(0.2f, 0.3f, 0.1f), m_rx_history);
        m_chart->add_series("TX Packets/s", Color(0.4f, 0.5f, 0.2f), m_tx_history);
        m_chart->invalidate();
    }
} // namespace horizon
