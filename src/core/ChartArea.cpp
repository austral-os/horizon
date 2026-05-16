#include <horizon/ChartArea.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Application.hpp>
#include <algorithm>
#include <cmath>

namespace horizon
{
    static std::vector<PolygonPoint> interpolate_points(const std::vector<PolygonPoint> &points, int segments)
    {
        if (points.size() < 2) return points;
        std::vector<PolygonPoint> result;
        
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            const auto &p0 = points[i == 0 ? i : i - 1];
            const auto &p1 = points[i];
            const auto &p2 = points[i + 1];
            const auto &p3 = points[i + 2 >= points.size() ? i + 1 : i + 2];

            for (int s = 0; s < segments; ++s)
            {
                double t = (double)s / segments;
                double t2 = t * t;
                double t3 = t2 * t;

                double f1 = -0.5 * t3 + t2 - 0.5 * t;
                double f2 = 1.5 * t3 - 2.5 * t2 + 1.0;
                double f3 = -1.5 * t3 + 2.0 * t2 + 0.5 * t;
                double f4 = 0.5 * t3 - 0.5 * t2;

                int x = (int)(p0.x * f1 + p1.x * f2 + p2.x * f3 + p3.x * f4);
                int y = (int)(p0.y * f1 + p1.y * f2 + p2.y * f3 + p3.y * f4);
                result.push_back(PolygonPoint(x, y));
            }
        }
        result.push_back(points.back());
        return result;
    }

    ChartArea::ChartArea() : ChartBase()
    {
    }

    void ChartArea::calculate_y_range(double &min_y, double &max_y)
    {
        if (!m_auto_scale)
        {
            min_y = m_fixed_min_y;
            max_y = m_fixed_max_y;
            return;
        }

        if (!m_stacked)
        {
            ChartBase::calculate_y_range(min_y, max_y);
            return;
        }

        if (m_percent_stacked)
        {
            min_y = 0;
            max_y = 100.0;
            return;
        }

        // Calculate Y range for stacked areas
        min_y = 0;
        max_y = 1.0;
        
        if (m_series.empty()) return;

        size_t num_points = 0;
        for (const auto &s : m_series) num_points = std::max(num_points, s->values.size());
        
        if (num_points == 0) return;

        bool first = true;
        for (size_t i = 0; i < num_points; ++i)
        {
            double sum_pos = 0;
            double sum_neg = 0;
            for (const auto &s : m_series)
            {
                if (!s->visible && s->current_opacity <= 0.01f) continue;
                if (i < s->values.size())
                {
                    if (s->values[i] >= 0) sum_pos += s->values[i];
                    else sum_neg += s->values[i];
                }
            }
            
            if (first) { min_y = sum_neg; max_y = sum_pos; first = false; }
            else { min_y = std::min(min_y, sum_neg); max_y = std::max(max_y, sum_pos); }
        }

        double range = max_y - min_y;
        if (range == 0) range = 1.0;
        max_y += range * 0.1;
        if (min_y != 0) min_y -= range * 0.1;
    }

    void ChartArea::draw(GraphicsContext &ctx)
    {
        if (m_series.empty()) return;

        double min_y, max_y;
        calculate_y_range(min_y, max_y);

        int plot_x = m_x + m_margin_left;
        int plot_y = m_y + m_margin_top;
        int plot_w = m_width - m_margin_left - m_margin_right;
        int plot_h = m_height - m_margin_top - m_margin_bottom;

        if (plot_w <= 0 || plot_h <= 0) return;

        // Draw Background
        ctx.setColor(m_bg_color);
        ctx.fillRect(m_x, m_y, m_width, m_height);

        draw_grid(ctx, plot_x, plot_y, plot_w, plot_h, min_y, max_y);
        draw_axes(ctx, plot_x, plot_y, plot_w, plot_h, min_y, max_y);

        size_t num_points = 0;
        for (const auto &s : m_series) num_points = std::max(num_points, s->values.size());
        if (num_points < 2) return;

        // Pre-calculate cumulative values for stacking
        std::vector<double> bottom_values(num_points, 0.0);
        std::vector<double> top_values(num_points, 0.0);

        for (size_t s_idx = 0; s_idx < m_series.size(); ++s_idx)
        {
            auto &series = m_series[s_idx];
            float opacity = series->current_opacity;
            if (opacity <= 0.01f) continue;

            std::vector<PolygonPoint> area_points;
            std::vector<PolygonPoint> line_points;

            for (size_t i = 0; i < num_points; ++i)
            {
                double val = (i < series->values.size()) ? series->values[i] : 0.0;
                double display_val = val;
                double bottom_val = 0;

                if (m_stacked)
                {
                    if (m_percent_stacked)
                    {
                        double total = 0;
                        for (const auto &os : m_series) 
                            if (os->visible && i < os->values.size()) total += os->values[i];
                        if (total != 0) {
                            display_val = (val / total) * 100.0;
                            bottom_val = (bottom_values[i] / total) * 100.0;
                        }
                    }
                    else
                    {
                        bottom_val = bottom_values[i];
                        display_val = bottom_val + val;
                    }
                    bottom_values[i] += val;
                }

                int x = plot_x + (i * plot_w / (num_points - 1));
                int y = plot_y + plot_h - (int)((display_val - min_y) * plot_h / (max_y - min_y));
                int by = plot_y + plot_h - (int)((bottom_val - min_y) * plot_h / (max_y - min_y));

                line_points.push_back(PolygonPoint(x, y));
            }

            if (m_smooth_curves)
            {
                line_points = interpolate_points(line_points, 5);
            }

            // Close the area polygon
            area_points = line_points;
            if (m_stacked)
            {
                std::vector<PolygonPoint> bottom_points;
                // Go back along the bottom values
                for (int i = num_points - 1; i >= 0; --i)
                {
                    double val = (i < series->values.size()) ? series->values[i] : 0.0;
                    double bottom_val = 0;
                    if (m_percent_stacked)
                    {
                         double total = 0;
                         for (const auto &os : m_series) 
                             if (os->visible && i < os->values.size()) total += os->values[i];
                         if (total != 0) bottom_val = ((bottom_values[i] - val) / total) * 100.0;
                    }
                    else
                    {
                        bottom_val = bottom_values[i] - val;
                    }
                    
                    int x = plot_x + (i * plot_w / (num_points - 1));
                    int y = plot_y + plot_h - (int)((bottom_val - min_y) * plot_h / (max_y - min_y));
                    bottom_points.push_back(PolygonPoint(x, y));
                }
                
                if (m_smooth_curves)
                {
                    bottom_points = interpolate_points(bottom_points, 5);
                }
                
                area_points.insert(area_points.end(), bottom_points.begin(), bottom_points.end());
            }
            else
            {
                int base_y = plot_y + plot_h - (int)((0.0 - min_y) * plot_h / (max_y - min_y));
                area_points.push_back(PolygonPoint(area_points.back().x, base_y));
                area_points.push_back(PolygonPoint(area_points.front().x, base_y));
            }

            draw_series_area(ctx, area_points, series->color, opacity);
            draw_series_line(ctx, line_points, series->color, opacity);
        }

        int legend_y = plot_y + plot_h + 35;
        draw_legend(ctx, plot_x, legend_y, plot_w);

        // Draw Tooltip
        if (m_hovered_data_idx >= 0)
        {
            int tx = plot_x + (m_hovered_data_idx * plot_w / (num_points - 1));
            
            // Vertical indicator line
            ctx.setColor(theme_manager()->get_color("window_fg").with_alpha(0.2f));
            ctx.drawLine(tx, plot_y, tx, plot_y + plot_h, 1.0f);
            
            // Draw indicators on each series
            for (size_t i = 0; i < m_series.size(); ++i)
            {
                if (!m_series[i]->visible && m_series[i]->current_opacity <= 0.01f) continue;
                
                // (Calculation of Y position logic duplicated for brevity, ideally refactor)
                // ... but for now, let's just draw the tooltip at the mouse position or at the nearest series
                if (m_hovered_series_idx == (int)i) {
                     // Find Y for this series at this point
                     double val = (m_hovered_data_idx < (int)m_series[i]->values.size()) ? m_series[i]->values[m_hovered_data_idx] : 0.0;
                     // ... simplified Y calculation for tooltip anchor
                     int ty = plot_y + plot_h - (int)((val - min_y) * plot_h / (max_y - min_y));
                     
                     ctx.setColor(m_series[i]->color);
                     ctx.fillCircle(tx, ty, 4);
                     ctx.setColor(Color(1,1,1,0.8f));
                     ctx.drawCircle(tx, ty, 4, 2.0f);
                     
                     draw_tooltip(ctx, tx, ty, i, m_hovered_data_idx);
                }
            }
        }
    }

    bool ChartArea::find_nearest_point(int x, int y, int &series_idx, int &data_idx)
    {
        if (m_series.empty()) return false;
        
        int plot_x = m_x + m_margin_left;
        int plot_w = m_width - m_margin_left - m_margin_right;
        
        if (x < plot_x || x > plot_x + plot_w) return false;
        
        size_t num_points = 0;
        for (const auto &s : m_series) num_points = std::max(num_points, s->values.size());
        if (num_points < 2) return false;
        
        data_idx = (int)std::round((double)(x - plot_x) * (num_points - 1) / plot_w);
        if (data_idx < 0 || data_idx >= (int)num_points) return false;
        
        // Find closest series
        double min_dist = 1000000;
        series_idx = -1;
        
        double min_y_range, max_y_range;
        calculate_y_range(min_y_range, max_y_range);
        int plot_y = m_y + m_margin_top;
        int plot_h = m_height - m_margin_top - m_margin_bottom;

        for (size_t i = 0; i < m_series.size(); ++i)
        {
            if (!m_series[i]->visible) continue;
            if (data_idx >= (int)m_series[i]->values.size()) continue;
            
            double val = m_series[i]->values[data_idx];
            int py = plot_y + plot_h - (int)((val - min_y_range) * plot_h / (max_y_range - min_y_range));
            
            double dist = std::abs(y - py);
            if (dist < min_dist)
            {
                min_dist = dist;
                series_idx = i;
            }
        }
        
        return series_idx != -1;
    }

    void ChartArea::draw_series_area(GraphicsContext &ctx, const std::vector<PolygonPoint> &points, const Color &color, float opacity)
    {
        if (points.empty()) return;
        
        Color c1 = color.with_alpha(0.5f * opacity);
        Color c2 = color.with_alpha(0.1f * opacity);
        
        ctx.fillLinearGradientPolygon(points, c1, c2, true);
    }

    void ChartArea::draw_series_line(GraphicsContext &ctx, const std::vector<PolygonPoint> &points, const Color &color, float opacity)
    {
        if (points.empty()) return;
        
        ctx.setColor(color.with_alpha(opacity));
        ctx.drawPolyline(points, 2.0f);
    }

} // namespace horizon
