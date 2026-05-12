#include "EnergyStats.hpp"
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    EnergyStats::EnergyStats() : Widget()
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
        left_panel->add_child(create_stat_row("Remaining Charge:", &m_lbl_remaining));
        left_panel->add_child(create_stat_row("Time until full:", &m_lbl_time_full));
        left_panel->add_child(create_stat_row("Time on AC:", &m_lbl_time_ac));

        // Center: The Chart
        auto chart_wrapper = std::make_unique<Widget>();
        chart_wrapper->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        chart_wrapper->set_position_type(FILL);

        auto chart = std::make_unique<ChartArea>();
        m_chart = chart.get();
        m_chart->set_smooth_curves(true);
        m_chart->set_show_grid(true);
        m_chart->set_show_legend(false);
        m_chart->set_show_axes(false);
        m_chart->set_y_range(0, 100);
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
        right_panel->add_child(create_stat_row("Time to empty:", &m_lbl_time_empty));
        right_panel->add_child(create_stat_row("Cycle Count:", &m_lbl_cycles));
        right_panel->add_child(create_stat_row("Battery Health:", &m_lbl_health));

        container->add_child(std::move(left_panel));
        container->add_child(std::move(chart_wrapper));
        container->add_child(std::move(right_panel));

        add_child(std::move(container));

        m_history.resize(m_max_history, 0.0);
    }

    std::unique_ptr<Widget> EnergyStats::create_stat_row(const std::string &name,
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

    void EnergyStats::update(const EnergyUsage &usage)
    {
        char buf[64];

        snprintf(buf, sizeof(buf), "%.1f%%", usage.percentage);
        if (m_lbl_remaining)
            m_lbl_remaining->set_text(buf);

        auto format_time = [](int mins)
        {
            if (mins <= 0)
                return std::string("-");
            char b[32];
            snprintf(b, sizeof(b), "%dh %dm", mins / 60, mins % 60);
            return std::string(b);
        };

        if (m_lbl_time_full)
            m_lbl_time_full->set_text(format_time(usage.time_to_full_mins));
        if (m_lbl_time_ac)
            m_lbl_time_ac->set_text(usage.on_ac ? "Connected" : "Disconnected");

        if (m_lbl_time_empty)
            m_lbl_time_empty->set_text(format_time(usage.time_to_empty_mins));

        snprintf(buf, sizeof(buf), "%d", usage.cycle_count);
        if (m_lbl_cycles)
            m_lbl_cycles->set_text(buf);

        snprintf(buf, sizeof(buf), "%.1f%%", usage.health_percent);
        if (m_lbl_health)
            m_lbl_health->set_text(buf);

        m_history.push_back(usage.percentage);
        if (m_history.size() > m_max_history)
            m_history.erase(m_history.begin());

        m_chart->clear_series();
        m_chart->add_series("Battery Charge", Color(0.2f, 0.3f, 0.1f), m_history);
        m_chart->invalidate();
    }
} // namespace horizon
