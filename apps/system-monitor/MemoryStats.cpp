#include "MemoryStats.hpp"
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    MemoryStats::MemoryStats() : Widget()
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
        left_panel->add_child(create_stat_row("Physical Memory:", &m_lbl_total_phys));
        left_panel->add_child(create_stat_row("Memory Used:", &m_lbl_used_phys));
        left_panel->add_child(create_stat_row("Virtual Memory:", &m_lbl_total_virt));
        left_panel->add_child(create_stat_row("Swap Used:", &m_lbl_used_swap));

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
        right_panel->add_child(create_stat_row("Available:", &m_lbl_available));
        right_panel->add_child(create_stat_row("Cached:", &m_lbl_cached));
        right_panel->add_child(create_stat_row("Free:", &m_lbl_free));
        right_panel->add_child(create_stat_row("Usage %:", &m_lbl_used_pct));

        container->add_child(std::move(left_panel));
        container->add_child(std::move(chart_wrapper));
        container->add_child(std::move(right_panel));

        add_child(std::move(container));

        m_history.resize(m_max_history, 0.0);
    }

    std::unique_ptr<Widget> MemoryStats::create_stat_row(const std::string &name,
                                                         Label **value_label_out)
    {
        auto row = std::make_unique<Widget>();
        row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row->set_fixed_size(25);

        auto name_lbl = std::make_unique<Label>(name);
        name_lbl->set_alignment(TextAlignment::Left);
        name_lbl->set_position_type(FILL);
        name_lbl->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        name_lbl->set_font_size(16);
        name_lbl->set_text_color(Color(0.2f, 0.3f, 0.1f, 1.0f));

        auto value_lbl = std::make_unique<Label>("0.00 GB");
        value_lbl->set_alignment(TextAlignment::Right);
        value_lbl->set_text_color(Color(0.2f, 0.3f, 0.1f, 1.0f));
        value_lbl->set_font_size(16);

        *value_label_out = value_lbl.get();

        row->add_child(std::move(name_lbl));
        row->add_child(std::move(value_lbl));

        return row;
    }

    void MemoryStats::update(const MemoryUsage &usage)
    {
        auto format_bytes = [](uint64_t bytes)
        {
            char buf[64];
            double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
            if (gb >= 1.0)
            {
                snprintf(buf, sizeof(buf), "%.2f GB", gb);
            }
            else
            {
                double mb = (double)bytes / (1024.0 * 1024.0);
                snprintf(buf, sizeof(buf), "%.1f MB", mb);
            }
            return std::string(buf);
        };

        if (m_lbl_total_phys)
            m_lbl_total_phys->set_text(format_bytes(usage.total_physical));
        if (m_lbl_used_phys)
            m_lbl_used_phys->set_text(format_bytes(usage.used_physical));
        if (m_lbl_total_virt)
            m_lbl_total_virt->set_text(format_bytes(usage.total_virtual));
        if (m_lbl_used_swap)
            m_lbl_used_swap->set_text(format_bytes(usage.used_swap));

        if (m_lbl_available)
            m_lbl_available->set_text(format_bytes(usage.available_physical));
        if (m_lbl_cached)
            m_lbl_cached->set_text(format_bytes(usage.cached));
        if (m_lbl_free)
            m_lbl_free->set_text(format_bytes(usage.free_physical));

        char pct_buf[32];
        snprintf(pct_buf, sizeof(pct_buf), "%.2f%%", usage.used_percent);
        if (m_lbl_used_pct)
            m_lbl_used_pct->set_text(pct_buf);

        m_history.push_back(usage.used_percent);
        if (m_history.size() > m_max_history)
            m_history.erase(m_history.begin());

        m_chart->clear_series();
        m_chart->add_series("Memory Usage", Color(0.2f, 0.3f, 0.1f), m_history);
        m_chart->invalidate();
    }
} // namespace horizon
