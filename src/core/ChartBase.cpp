#include <horizon/ChartBase.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Application.hpp>
#include <horizon/WaylandWindow.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace horizon
{
    ChartBase::ChartBase() : Widget()
    {
        when_mouse_move.connect([this](MouseMoveEventContext &ctx) {
            handle_mouse_move(ctx.x, ctx.y);
        });

        when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
            handle_mouse_press(ctx.x, ctx.y, ctx.button);
        });
        
        when_mouse_leave.connect([this](EventContext &ctx) {
            m_hovered_series_idx = -1;
            m_hovered_data_idx = -1;
            invalidate();
        });
    }

    ChartBase::~ChartBase()
    {
        if (m_app && m_animation_timer_id != 0)
        {
            m_app->stop_timer(m_animation_timer_id);
            m_animation_timer_id = 0;
        }
    }

    void ChartBase::add_series(const std::string &name, const Color &color, const std::vector<double> &values)
    {
        auto series = std::make_unique<ChartSeries>();
        series->name = name;
        series->color = color;
        series->values = values;
        series->current_values = values; // Start at target values
        series->visible = true;
        series->opacity = 1.0f;
        series->current_opacity = 1.0f;
        
        m_series.push_back(std::move(series));
        invalidate();
    }

    void ChartBase::update_series_values(const std::string &name, const std::vector<double> &values)
    {
        for (auto &s : m_series)
        {
            if (s->name == name)
            {
                s->values = values;
                // If not animating, sync current_values too
                if (m_animation_timer_id == 0) s->current_values = values;
                invalidate();
                return;
            }
        }
    }

    void ChartBase::remove_series(const std::string &name)
    {
        m_series.erase(std::remove_if(m_series.begin(), m_series.end(),
                                     [&name](const auto &s) { return s->name == name; }),
                       m_series.end());
        invalidate();
    }

    void ChartBase::clear_series()
    {
        m_series.clear();
        invalidate();
    }

    void ChartBase::set_labels(const std::vector<std::string> &labels)
    {
        m_labels = labels;
        invalidate();
    }

    void ChartBase::set_y_range(double min_y, double max_y)
    {
        m_fixed_min_y = min_y;
        m_fixed_max_y = max_y;
        m_auto_scale = false;
        invalidate();
    }

    void ChartBase::calculate_y_range(double &min_y, double &max_y)
    {
        if (!m_auto_scale)
        {
            min_y = m_fixed_min_y;
            max_y = m_fixed_max_y;
            return;
        }

        min_y = 0;
        max_y = 1.0;
        bool first = true;

        for (const auto &s : m_series)
        {
            if (!s->visible && s->current_opacity <= 0.01f) continue;
            
            for (double val : s->values)
            {
                if (first)
                {
                    min_y = val;
                    max_y = val;
                    first = false;
                }
                else
                {
                    min_y = std::min(min_y, val);
                    max_y = std::max(max_y, val);
                }
            }
        }

        // Add some padding
        double range = max_y - min_y;
        if (range == 0) range = 1.0;
        
        max_y += range * 0.1;
        if (min_y != 0) min_y -= range * 0.1;
    }

    void ChartBase::draw_grid(GraphicsContext &ctx, int px, int py, int pw, int ph, double min_y, double max_y)
    {
        if (!m_show_grid) return;

        auto *tm = theme_manager();
        ctx.setColor(tm->get_color("window_fg").with_alpha(0.1f));

        int num_ticks = 5;
        for (int i = 0; i <= num_ticks; ++i)
        {
            int y = py + ph - (i * ph / num_ticks);
            ctx.drawLine(px, y, px + pw, y, 1.0f);
        }
    }

    void ChartBase::draw_axes(GraphicsContext &ctx, int px, int py, int pw, int ph, double min_y, double max_y)
    {
        if (!m_show_axes) return;
        
        auto *tm = theme_manager();
        auto theme_font = tm->get_font("window");
        
        ctx.setColor(tm->get_color("window_fg").with_alpha(0.6f));
        ctx.setDrawFont(theme_font.family.c_str(), theme_font.size - 2, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        // Y Axis Ticks
        int num_ticks = 5;
        for (int i = 0; i <= num_ticks; ++i)
        {
            double val = min_y + (i * (max_y - min_y) / num_ticks);
            std::string label = format_value(val);
            int y = py + ph - (i * ph / num_ticks);
            
            auto metrics = ctx.getTextMetrics(label.c_str(), theme_font.family.c_str(), theme_font.size - 2, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            ctx.drawText(px - metrics.width - 10, y + (metrics.height / 2), label.c_str());
        }

        // X Axis Labels
        if (!m_labels.empty())
        {
            int num_labels = m_labels.size();
            for (int i = 0; i < num_labels; ++i)
            {
                int x = px + (i * pw / (num_labels - 1));
                
                auto metrics = ctx.getTextMetrics(m_labels[i].c_str(), theme_font.family.c_str(), theme_font.size - 2, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                ctx.drawText(x - (metrics.width / 2), py + ph + metrics.height + 10, m_labels[i].c_str());
            }
        }
        
        // Main Axis lines
        ctx.setColor(tm->get_color("window_fg").with_alpha(0.3f));
        ctx.drawLine(px, py, px, py + ph, 1.0f); // Y
        ctx.drawLine(px, py + ph, px + pw, py + ph, 1.0f); // X
    }

    int ChartBase::draw_legend(GraphicsContext &ctx, int lx, int ly, int lw)
    {
        if (!m_show_legend || m_series.empty()) return 0;

        auto *tm = theme_manager();
        auto theme_font = tm->get_font("window");
        
        m_legend_items.clear();
        
        int item_x = lx;
        int item_y = ly;
        int spacing = 20;
        int indicator_size = 12;
        int text_spacing = 8;
        int row_height = 20;

        for (size_t i = 0; i < m_series.size(); ++i)
        {
            const auto &s = m_series[i];
            
            auto metrics = ctx.getTextMetrics(s->name.c_str(), theme_font.family.c_str(), theme_font.size - 1, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
            int item_width = indicator_size + text_spacing + metrics.width;

            if (item_x + item_width > lx + lw)
            {
                item_x = lx;
                item_y += row_height;
            }

            // Indicator
            ctx.setColor(s->color.with_alpha(s->visible ? 1.0f : 0.3f));
            ctx.fillRect(item_x, item_y + (row_height - indicator_size) / 2, indicator_size, indicator_size, 3);
            
            // Name
            ctx.setColor(tm->get_color("window_fg").with_alpha(s->visible ? 1.0f : 0.4f));
            ctx.setDrawFont(theme_font.family.c_str(), theme_font.size - 1, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
            ctx.drawText(item_x + indicator_size + text_spacing, item_y + theme_font.size, s->name.c_str());
            
            m_legend_items.push_back({item_x, item_y, item_width, row_height, i});
            
            item_x += item_width + spacing;
        }

        return (item_y - ly) + row_height;
    }

    void ChartBase::draw_tooltip(GraphicsContext &ctx, int x, int y, int series_idx, int data_idx)
    {
        if (!m_show_tooltip || series_idx < 0 || series_idx >= (int)m_series.size()) return;
        
        auto *tm = theme_manager();
        auto theme_font = tm->get_font("window");
        const auto &s = m_series[series_idx];
        
        std::string val_str = format_value(s->values[data_idx]);
        std::string label = (data_idx < (int)m_labels.size()) ? m_labels[data_idx] : "";
        
        std::string text = s->name + ": " + val_str;
        if (!label.empty()) text = label + " - " + text;

        ctx.setDrawFont(theme_font.family.c_str(), theme_font.size - 1, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        auto metrics = ctx.getTextMetrics(text.c_str(), theme_font.family.c_str(), theme_font.size - 1, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        
        int tw = metrics.width + 20;
        int th = metrics.height + 14;
        int tx = x - tw / 2;
        int ty = y - th - 10;
        
        // Keep inside widget
        if (tx < m_x) tx = m_x;
        if (tx + tw > m_x + m_width) tx = m_x + m_width - tw;
        if (ty < m_y) ty = y + 20;

        // Tooltip box
        ctx.setColor(tm->get_color("window_bg").with_alpha(0.9f));
        ctx.fillRect(tx, ty, tw, th, 6);
        ctx.setColor(s->color.with_alpha(0.6f));
        ctx.drawRect(tx, ty, tw, th, 6, 1.0f);
        
        // Text
        ctx.setColor(tm->get_color("window_fg"));
        ctx.drawText(tx + 10, ty + th - 10, text.c_str());
    }

    void ChartBase::handle_mouse_move(double x, double y)
    {
        m_mouse_x = (int)x;
        m_mouse_y = (int)y;
        
        int old_s = m_hovered_series_idx;
        int old_d = m_hovered_data_idx;
        
        if (!find_nearest_point(m_mouse_x, m_mouse_y, m_hovered_series_idx, m_hovered_data_idx))
        {
            m_hovered_series_idx = -1;
            m_hovered_data_idx = -1;
        }
        
        if (old_s != m_hovered_series_idx || old_d != m_hovered_data_idx)
        {
            invalidate();
        }
    }

    void ChartBase::handle_mouse_press(double x, double y, uint32_t button)
    {
        for (const auto &item : m_legend_items)
        {
            if (x >= item.x && x <= item.x + item.w && y >= item.y && y <= item.y + item.h)
            {
                m_series[item.series_idx]->visible = !m_series[item.series_idx]->visible;
                start_animation();
                invalidate();
                return;
            }
        }
    }

    std::string ChartBase::format_value(double value) const
    {
        std::stringstream ss;
        if (std::abs(value) >= 1000000) ss << std::fixed << std::setprecision(1) << (value / 1000000.0) << "M";
        else if (std::abs(value) >= 1000) ss << std::fixed << std::setprecision(1) << (value / 1000.0) << "K";
        else if (std::abs(value) < 0.1 && value != 0) ss << std::fixed << std::setprecision(3) << value;
        else ss << std::fixed << std::setprecision(1) << value;
        
        std::string s = ss.str();
        // Remove trailing .0
        if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s = s.substr(0, s.size() - 2);
        return s;
    }

    void ChartBase::start_animation()
    {
        if (m_animation_timer_id != 0) return;
        
        if (m_app)
        {
            m_animation_timer_id = m_app->add_timer(16, [this]() {
                update_animations();
            }, true);
        }
    }

    void ChartBase::update_animations()
    {
        bool still_animating = false;
        float step = 0.1f;

        for (auto &s : m_series)
        {
            // Opacity animation
            float target_opacity = s->visible ? 1.0f : 0.0f;
            if (std::abs(s->current_opacity - target_opacity) > 0.01f)
            {
                s->current_opacity += (target_opacity - s->current_opacity) * step;
                still_animating = true;
            }
            else
            {
                s->current_opacity = target_opacity;
            }

            // Value animation (optional, for later)
        }

        invalidate();

        if (!still_animating && m_animation_timer_id != 0)
        {
            m_app->stop_timer(m_animation_timer_id);
            m_animation_timer_id = 0;
        }
    }

} // namespace horizon
