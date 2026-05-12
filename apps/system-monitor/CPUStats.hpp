#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ChartArea.hpp>
#include "ProcessManager.hpp"
#include <memory>
#include <vector>

namespace horizon
{
    /**
     * @brief Widget that displays CPU statistics using a ChartArea.
     */
    class CPUStats : public Widget
    {
    public:
        CPUStats();
        virtual ~CPUStats() = default;

        /**
         * @brief Updates the chart with new CPU usage data.
         */
        void update(const CPUUsage &usage);

        /**
         * @brief Toggles between showing total CPU usage or individual cores.
         */
        void set_show_cores(bool show);
        
        bool showing_cores() const { return m_show_cores; }

    private:
        void init_chart();
        void update_series();

        ChartArea* m_chart{nullptr};
        bool m_show_cores{false};
        
        // History data: [series_index][time_point]
        // series 0: total
        // series 1..N: cores
        std::vector<std::vector<double>> m_history;
        size_t m_max_history{60};
        
        std::vector<Color> m_core_colors;
    };
} // namespace horizon
