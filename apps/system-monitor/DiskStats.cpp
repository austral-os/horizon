#include "DiskStats.hpp"
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    DiskStats::DiskStats() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(15);
        set_margin(20);

        auto container = std::make_unique<AquaObject>();
        container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        container->set_accent_color(WidgetAccentColor::Custom);
        auto update_container_colors = [container_ptr = container.get()]() {
            Color col1 = Color(0.8f, 0.9f, 0.4f, 1.0f);
            Color col2 = Color(0.8f, 0.8f, 0.4f, 1.0f);

            if (theme_manager()->get_variant() == "dark")
            {
                col1 = Color(0.22f, 0.26f, 0.16f, 1.0f);
                col2 = Color(0.13f, 0.16f, 0.10f, 1.0f);
            }

            container_ptr->set_color1(col1);
            container_ptr->set_color2(col2);
            container_ptr->invalidate();
        };
        update_container_colors();

        theme_manager()->when_change.connect(
            [update_container_colors](ThemeEventContext &) {
                update_container_colors();
            });

        container->set_border_width(1);
        container->set_corner_radius(15);

        // Left Panel
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_panel->set_fixed_size(300);
        left_panel->set_margin(15);
        left_panel->set_spacing(5);
        left_panel->add_child(create_stat_row("Reads in:", &m_lbl_reads_completed));
        left_panel->add_child(create_stat_row("Writes out:", &m_lbl_writes_completed));
        left_panel->add_child(create_stat_row("Reads in/sec:", &m_lbl_read_speed));
        left_panel->add_child(create_stat_row("Writes out/sec:", &m_lbl_write_speed));

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
        right_panel->add_child(create_stat_row("Data Read:", &m_lbl_total_read));
        right_panel->add_child(create_stat_row("Data Written:", &m_lbl_total_written));
        right_panel->add_child(create_stat_row("Read Speed:", &m_lbl_read_speed_r));
        right_panel->add_child(create_stat_row("Write Speed:", &m_lbl_write_speed_r));

        container->add_child(std::move(left_panel));
        container->add_child(std::move(chart_wrapper));
        container->add_child(std::move(right_panel));

        add_child(std::move(container));

        m_read_history.resize(m_max_history, 0.0);
        m_write_history.resize(m_max_history, 0.0);
    }

    std::unique_ptr<Widget> DiskStats::create_stat_row(const std::string &name,
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

        auto value_lbl = std::make_unique<Label>("-");
        value_lbl->set_alignment(TextAlignment::Right);
        value_lbl->set_font_size(16);

        *value_label_out = value_lbl.get();

        auto update_label_colors = [name_lbl_ptr = name_lbl.get(), value_lbl_ptr = value_lbl.get()]() {
            Color lbl_color = Color(0.2f, 0.3f, 0.1f, 1.0f);
            if (theme_manager()->get_variant() == "dark")
            {
                lbl_color = Color(0.8f, 0.9f, 0.4f, 1.0f);
            }
            name_lbl_ptr->set_text_color(lbl_color);
            value_lbl_ptr->set_text_color(lbl_color);
            name_lbl_ptr->invalidate();
            value_lbl_ptr->invalidate();
        };
        update_label_colors();

        theme_manager()->when_change.connect(
            [update_label_colors](ThemeEventContext &) {
                update_label_colors();
            });

        row->add_child(std::move(name_lbl));
        row->add_child(std::move(value_lbl));

        return row;
    }

    void DiskStats::update(const DiskUsage &usage)
    {
        char buf[64];

        snprintf(buf, sizeof(buf), "%lu", usage.reads_completed);
        if (m_lbl_reads_completed)
            m_lbl_reads_completed->set_text(buf);

        snprintf(buf, sizeof(buf), "%lu", usage.writes_completed);
        if (m_lbl_writes_completed)
            m_lbl_writes_completed->set_text(buf);

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

        std::string r_speed = format_speed(usage.read_speed_kb);
        std::string w_speed = format_speed(usage.write_speed_kb);

        if (m_lbl_read_speed)
            m_lbl_read_speed->set_text(r_speed);
        if (m_lbl_write_speed)
            m_lbl_write_speed->set_text(w_speed);
        if (m_lbl_read_speed_r)
            m_lbl_read_speed_r->set_text(r_speed);
        if (m_lbl_write_speed_r)
            m_lbl_write_speed_r->set_text(w_speed);

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

        if (m_lbl_total_read)
            m_lbl_total_read->set_text(format_bytes(usage.total_read_bytes));
        if (m_lbl_total_written)
            m_lbl_total_written->set_text(format_bytes(usage.total_written_bytes));

        m_read_history.push_back(usage.read_speed_kb);
        if (m_read_history.size() > m_max_history)
            m_read_history.erase(m_read_history.begin());

        m_write_history.push_back(usage.write_speed_kb);
        if (m_write_history.size() > m_max_history)
            m_write_history.erase(m_write_history.begin());

        Color read_color = Color(0.2f, 0.3f, 0.1f);
        Color write_color = Color(0.4f, 0.5f, 0.2f);
        if (theme_manager()->get_variant() == "dark")
        {
            read_color = Color(0.8f, 0.9f, 0.4f);
            write_color = Color(0.6f, 0.8f, 0.3f);
        }

        m_chart->clear_series();
        m_chart->add_series("Read Speed", read_color, m_read_history);
        m_chart->add_series("Write Speed", write_color, m_write_history);
        m_chart->invalidate();
    }
} // namespace horizon
