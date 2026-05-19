#include "CPUStats.hpp"
#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    CPUStats::CPUStats() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(15);
        set_margin(20);

        auto container = std::make_unique<AquaObject>();
        container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        // container->set_spacing(20);
        // container->set_margin(20);
        //  container->set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
        //  container->set_border_color(Color(0.5f, 0.5f, 0.5f, 1.0f));

        Color col1 = Color(0.8f, 0.9f, 0.4f, 1.0f);
        Color col2 = Color(0.8f, 0.8f, 0.4f, 1.0f);

        if (theme_manager()->get_variant() == "dark")
        {
            // Sleek deep sage/olive gradient optimized for dark mode
            col1 = Color(0.22f, 0.26f, 0.16f, 1.0f);
            col2 = Color(0.13f, 0.16f, 0.10f, 1.0f);
        }

        container->set_accent_color(WidgetAccentColor::Custom);
        container->set_color1(col1);
        container->set_color2(col2);
        container->set_border_width(1);
        container->set_corner_radius(15);

        // Left Panel: CPU load details
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_panel->set_fixed_size(220);
        left_panel->set_margin(15);
        left_panel->set_spacing(5);
        left_panel->add_child(create_stat_row("System:", &m_lbl_system));
        left_panel->add_child(create_stat_row("User:", &m_lbl_user));
        left_panel->add_child(create_stat_row("Idle:", &m_lbl_idle));

        // Center: The Chart
        auto chart_wrapper = std::make_unique<Widget>();
        chart_wrapper->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        chart_wrapper->set_position_type(FILL);

        auto chart = std::make_unique<ChartArea>();
        m_chart = chart.get();
        m_chart->set_smooth_curves(true);
        m_chart->set_show_grid(true);
        m_chart->set_show_legend(false); // Legend might be too big here, stats already show info
        m_chart->set_show_axes(false);
        m_chart->set_y_range(0, 100);
        m_chart->set_margins(10, 10, 10, 10);
        m_chart->set_position_type(FILL);

        // Set white background for the chart
        m_chart->set_bg_color(Color(1.0f, 1.0f, 1.0f, 0.0f));

        chart_wrapper->add_child(std::move(chart));

        m_chart->when_mouse_press.connect([this](MouseButtonEventContext &)
                                          { set_show_cores(!m_show_cores); });

        // Right Panel: Global stats
        auto right_panel = std::make_unique<Widget>();
        right_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_panel->set_fixed_size(220);
        right_panel->set_margin(15);
        right_panel->set_spacing(5);
        right_panel->add_child(create_stat_row("Threads:", &m_lbl_threads));
        right_panel->add_child(create_stat_row("Processes:", &m_lbl_processes));

        m_core_colors = {Color(0.2f, 0.3f, 0.1f),    Color(0.3f, 0.4f, 0.15f),
                         Color(0.4f, 0.5f, 0.2f),    Color(0.15f, 0.25f, 0.05f),
                         Color(0.25f, 0.35f, 0.12f), Color(0.35f, 0.45f, 0.18f),
                         Color(0.1f, 0.2f, 0.05f),   Color(0.45f, 0.55f, 0.22f)};

        container->add_child(std::move(left_panel));
        container->add_child(std::move(chart_wrapper));
        container->add_child(std::move(right_panel));

        add_child(std::move(container));
    }

    std::unique_ptr<Widget> CPUStats::create_stat_row(const std::string &name,
                                                      Label **value_label_out)
    {

        Color lbl_color = Color(0.2f, 0.3f, 0.1f, 1.0f);

        if (theme_manager()->get_variant() == "dark")
        {
            lbl_color = Color(0.8f, 0.9f, 0.4f, 1.0f);
        }

        auto row = std::make_unique<Widget>();
        row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row->set_fixed_size(25);

        auto name_lbl = std::make_unique<Label>(name);
        name_lbl->set_alignment(TextAlignment::Left);
        name_lbl->set_position_type(FILL);
        name_lbl->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        name_lbl->set_text_color(lbl_color);
        name_lbl->set_font_size(16);

        auto value_lbl = std::make_unique<Label>("0.00%");
        value_lbl->set_alignment(TextAlignment::Right);
        value_lbl->set_text_color(lbl_color);
        value_lbl->set_font_size(16);

        *value_label_out = value_lbl.get();

        row->add_child(std::move(name_lbl));
        row->add_child(std::move(value_lbl));

        return row;
    }

    void CPUStats::update(const CPUUsage &usage)
    {
        // Update Labels
        auto format_pct = [](double v)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f%%", v);
            return std::string(buf);
        };

        if (m_lbl_system)
            m_lbl_system->set_text(format_pct(usage.system));
        if (m_lbl_user)
            m_lbl_user->set_text(format_pct(usage.user));
        if (m_lbl_idle)
            m_lbl_idle->set_text(format_pct(usage.idle));

        if (m_lbl_threads)
            m_lbl_threads->set_text(std::to_string(usage.total_threads));
        if (m_lbl_processes)
            m_lbl_processes->set_text(std::to_string(usage.total_processes));

        // Ensure history has enough series
        size_t total_series = 1 + usage.cores.size();
        if (m_history.size() < total_series)
        {
            m_history.resize(total_series, std::vector<double>(m_max_history, 0.0));
        }

        // Add new points to history
        m_history[0].push_back(usage.total);
        if (m_history[0].size() > m_max_history)
            m_history[0].erase(m_history[0].begin());

        for (size_t i = 0; i < usage.cores.size(); ++i)
        {
            m_history[i + 1].push_back(usage.cores[i]);
            if (m_history[i + 1].size() > m_max_history)
                m_history[i + 1].erase(m_history[i + 1].begin());
        }

        update_series();
    }

    void CPUStats::set_show_cores(bool show)
    {
        if (m_show_cores == show)
            return;
        m_show_cores = show;
        update_series();
    }

    void CPUStats::update_series()
    {
        if (m_history.empty())
            return;

        m_chart->clear_series();

        if (!m_show_cores)
        {
            m_chart->add_series("CPU Total", Color(0.2f, 0.3f, 0.1f), m_history[0]);
        }
        else
        {
            for (size_t i = 1; i < m_history.size(); ++i)
            {
                Color c = m_core_colors[(i - 1) % m_core_colors.size()];
                m_chart->add_series("Core " + std::to_string(i - 1), c, m_history[i]);
            }
        }

        m_chart->invalidate();
    }
} // namespace horizon
