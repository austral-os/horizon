#pragma once

#include <horizon/ChartBase.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{
    /**
     * @brief A widget that displays data as an area chart.
     * 
     * Supports multiple series, stacked areas, and 100% stacked areas.
     * Features smooth curves, semi-transparent gradients, and interactivity.
     */
    class ChartArea : public ChartBase
    {
    public:
        ChartArea();
        virtual ~ChartArea() = default;

        /**
         * @brief Enables or disables stacked area mode.
         */
        void set_stacked(bool stacked) { m_stacked = stacked; invalidate(); }
        
        /**
         * @brief Enables or disables 100% stacked area mode.
         * If true, also enables stacked mode.
         */
        void set_percent_stacked(bool percent) { 
            m_percent_stacked = percent; 
            if (percent) m_stacked = true;
            invalidate(); 
        }

        virtual void draw(GraphicsContext &ctx) override;

    protected:
        /**
         * @brief Calculates the Y range specifically for stacked modes.
         */
        virtual void calculate_y_range(double &min_y, double &max_y) override;
        
        /**
         * @brief Finds the data point nearest to the given coordinates.
         */
        virtual bool find_nearest_point(int x, int y, int &series_idx, int &data_idx) override;

    private:
        bool m_stacked{false};
        bool m_percent_stacked{false};

        /**
         * @brief Draws a single series area.
         */
        void draw_series_area(GraphicsContext &ctx, const std::vector<PolygonPoint> &points, const Color &color, float opacity);
        
        /**
         * @brief Draws a single series line.
         */
        void draw_series_line(GraphicsContext &ctx, const std::vector<PolygonPoint> &points, const Color &color, float opacity);
    };

} // namespace horizon
