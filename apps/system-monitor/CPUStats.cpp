#include "CPUStats.hpp"
#include <horizon/ThemeManager.hpp>
#include <horizon/Application.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    CPUStats::CPUStats() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        
        auto chart = std::make_unique<ChartArea>();
        m_chart = chart.get();
        
        m_chart->set_smooth_curves(true);
        m_chart->set_show_grid(true);
        m_chart->set_show_legend(true);
        m_chart->set_y_range(0, 100);
        m_chart->set_position_type(FILL);
        
        add_child(std::move(chart));

        m_chart->when_mouse_press.connect([this](MouseButtonEventContext &) {
            set_show_cores(!m_show_cores);
        });

        m_core_colors = {
            Color(0.2f, 0.6f, 1.0f), Color(0.2f, 0.8f, 0.4f), 
            Color(1.0f, 0.6f, 0.2f), Color(0.8f, 0.2f, 0.8f),
            Color(0.4f, 0.4f, 1.0f), Color(1.0f, 0.2f, 0.2f),
            Color(0.2f, 1.0f, 1.0f), Color(0.8f, 0.8f, 0.2f)
        };
        
        // Init labels for 60 seconds
        std::vector<std::string> labels;
        for (int i = 60; i >= 0; i -= 10) {
            labels.push_back(std::to_string(i) + "s");
        }
        // m_chart->set_labels(labels); // We'll update labels later if needed
    }

    void CPUStats::update(const CPUUsage &usage)
    {
        // Ensure history has enough series
        size_t total_series = 1 + usage.cores.size();
        if (m_history.size() < total_series)
        {
            m_history.resize(total_series, std::vector<double>(m_max_history, 0.0));
        }

        // Add new points to history
        m_history[0].push_back(usage.total);
        if (m_history[0].size() > m_max_history) m_history[0].erase(m_history[0].begin());

        for (size_t i = 0; i < usage.cores.size(); ++i)
        {
            m_history[i + 1].push_back(usage.cores[i]);
            if (m_history[i + 1].size() > m_max_history) m_history[i + 1].erase(m_history[i + 1].begin());
        }

        update_series();
    }

    void CPUStats::set_show_cores(bool show)
    {
        if (m_show_cores == show) return;
        m_show_cores = show;
        update_series();
    }

    void CPUStats::update_series()
    {
        if (m_history.empty()) return;

        m_chart->clear_series();

        if (!m_show_cores)
        {
            m_chart->add_series("CPU Total", Color(0.2f, 0.6f, 1.0f), m_history[0]);
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
