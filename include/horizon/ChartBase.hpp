#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Color.hpp>
#include <vector>
#include <string>
#include <memory>

namespace horizon
{
    /**
     * @brief Represents a single data series in a chart.
     */
    struct ChartSeries
    {
        std::string name;
        Color color;
        std::vector<double> values;
        bool visible = true;
        
        /**
         * @brief Interpolated values used during animations.
         */
        std::vector<double> current_values;
        
        /**
         * @brief Target opacity for the series, used for fade animations.
         */
        float opacity = 1.0f;
        float current_opacity = 1.0f;
    };

    /**
     * @brief Abstract base class for chart widgets in Horizon.
     * 
     * Handles common charting functionality such as data management, 
     * axis scaling, grid rendering, and legend management.
     */
    class ChartBase : public Widget
    {
    public:
        ChartBase();
        virtual ~ChartBase();

        /**
         * @brief Adds a new data series to the chart.
         */
        void add_series(const std::string &name, const Color &color, const std::vector<double> &values);
        
        /**
         * @brief Updates the values of an existing series.
         */
        void update_series_values(const std::string &name, const std::vector<double> &values);
        
        /**
         * @brief Removes a series by name.
         */
        void remove_series(const std::string &name);
        
        /**
         * @brief Clears all data series.
         */
        void clear_series();
        
        /**
         * @brief Sets the background color of the chart.
         */
        void set_bg_color(const Color &color) { m_bg_color = color; invalidate(); }
        
        /**
         * @brief Sets the margins around the plot area.
         */
        void set_labels(const std::vector<std::string> &labels);
        
        /**
         * @brief Configures visual features.
         */
        void set_show_grid(bool show) { m_show_grid = show; invalidate(); }
        void set_show_legend(bool show) { m_show_legend = show; invalidate(); }
        void set_show_tooltip(bool show) { m_show_tooltip = show; invalidate(); }
        void set_smooth_curves(bool smooth) { m_smooth_curves = smooth; invalidate(); }
        void set_auto_scale(bool auto_scale) { m_auto_scale = auto_scale; invalidate(); }

        /**
         * @brief Manually sets the Y-axis range if auto-scaling is disabled.
         */
        void set_y_range(double min_y, double max_y);

        virtual void draw(GraphicsContext &ctx) override = 0;

        virtual int preferred_width() const override { return 400; }
        virtual int preferred_height() const override { return 300; }

    protected:
        /**
         * @brief Calculates the Y-axis range based on visible series.
         */
        virtual void calculate_y_range(double &min_y, double &max_y);
        
        /**
         * @brief Draws the background grid.
         */
        void draw_grid(GraphicsContext &ctx, int px, int py, int pw, int ph, double min_y, double max_y);
        
        /**
         * @brief Draws the chart legend and returns its height.
         */
        int draw_legend(GraphicsContext &ctx, int lx, int ly, int lw);
        
        /**
         * @brief Draws the X and Y axes with labels.
         */
        void draw_axes(GraphicsContext &ctx, int px, int py, int pw, int ph, double min_y, double max_y);

        /**
         * @brief Draws a tooltip at the given location.
         */
        void draw_tooltip(GraphicsContext &ctx, int x, int y, int series_idx, int data_idx);

        /**
         * @brief Finds the data point nearest to the given coordinates.
         */
        virtual bool find_nearest_point(int x, int y, int &series_idx, int &data_idx) { return false; }

        /**
         * @brief Helper to format numeric values for axis labels.
         */
        std::string format_value(double value) const;

        /**
         * @brief Handles mouse movement for tooltip detection.
         */
        void handle_mouse_move(double x, double y);
        
        /**
         * @brief Handles mouse clicks for legend interaction.
         */
        void handle_mouse_press(double x, double y, uint32_t button);

    protected:
        std::vector<std::unique_ptr<ChartSeries>> m_series;
        std::vector<std::string> m_labels;
        
        bool m_show_grid{true};
        bool m_show_legend{true};
        bool m_show_tooltip{true};
        Color m_bg_color{1.0f, 1.0f, 1.0f, 1.0f}; // Default white
        
        bool m_auto_scale{true};
        bool m_smooth_curves{false};
        
        double m_fixed_min_y{0.0};
        double m_fixed_max_y{100.0};
        
        // Layout margins
        int m_margin_left{60};
        int m_margin_right{20};
        int m_margin_top{20};
        int m_margin_bottom{40};
        
        // Interaction state
        int m_hovered_series_idx{-1};
        int m_hovered_data_idx{-1};
        int m_mouse_x{0};
        int m_mouse_y{0};
        
        struct LegendItem {
            int x, y, w, h;
            size_t series_idx;
        };
        std::vector<LegendItem> m_legend_items;
        
        // Animation support
        size_t m_animation_timer_id{0};
        void start_animation();
        void update_animations();
    };

} // namespace horizon
