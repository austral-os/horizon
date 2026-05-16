#include <algorithm>
#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CategorizedBar.hpp>
#include <horizon/FormatUtils.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <iomanip>
#include <sstream>

namespace horizon
{
    CategorizedBar::CategorizedBar() : Widget()
    {
        m_formatter = default_formatter;
    }

    void CategorizedBar::add_category(const std::string &name, double value, const Color &color)
    {
        m_categories.push_back({name, value, color});
        invalidate();
    }

    void CategorizedBar::clear_categories()
    {
        m_categories.clear();
        invalidate();
    }

    void CategorizedBar::set_total_value(double total)
    {
        m_total_value = total;
        invalidate();
    }

    void CategorizedBar::set_value_formatter(std::function<std::string(double)> formatter)
    {
        m_formatter = formatter;
        invalidate();
    }

    std::string CategorizedBar::default_formatter(double value)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << value;
        return ss.str();
    }

    int CategorizedBar::preferred_width() const
    {
        return 400; // Default reasonable width
    }

    int CategorizedBar::preferred_height() const
    {
        return 80;
    }

    int CategorizedBar::preferred_height(int width) const
    {
        // Simple heuristic: if width is small, we might need more height for legend wrapping.
        // For now, let's keep it simple.
        return 80;
    }

    void CategorizedBar::draw(GraphicsContext &gc)
    {
        if (m_categories.empty())
            return;

        auto *tm = theme_manager();
        auto theme_font = tm->get_font("window");

        // Calculate total
        double current_sum = 0;
        for (const auto &cat : m_categories)
        {
            current_sum += cat.value;
        }
        double total = (m_total_value > 0) ? m_total_value : current_sum;
        if (total <= 0)
            total = 1.0;

        int bar_height = 18;
        int bar_y = m_y;
        int bar_rx = 6;

        // Draw background track
        gc.setColor(tm->get_color("window_bg").darker(15));
        gc.fillRect(m_x, bar_y, m_width, bar_height, bar_rx);

        // Draw segments
        double current_x_offset = 0;
        for (size_t i = 0; i < m_categories.size(); ++i)
        {
            const auto &cat = m_categories[i];
            double segment_width = (cat.value / total) * m_width;

            // Avoid gaps between segments
            if (i == m_categories.size() - 1 && m_total_value <= 0)
            {
                segment_width = m_width - current_x_offset;
            }

            if (segment_width > 0.5) // Only draw if significant
            {
                gc.setColor(cat.color);

                CornerRadius radius(0);
                if (i == 0)
                {
                    radius.top_left = bar_rx;
                    radius.bottom_left = bar_rx;
                }

                // If this is the last one OR it reaches the end of the total bar
                bool is_at_end = (i == m_categories.size() - 1);
                if (is_at_end && std::abs(current_x_offset + segment_width - m_width) < 1.0)
                {
                    radius.top_right = bar_rx;
                    radius.bottom_right = bar_rx;
                }

                gc.fillRect(m_x + (int)std::round(current_x_offset), bar_y,
                            (int)std::ceil(segment_width), bar_height, radius);
                current_x_offset += segment_width;
            }
        }

        // Draw Legend
        int legend_y = bar_y + bar_height + 15;
        int item_x = m_x;
        int item_spacing = 24;
        int indicator_size = 10;
        int text_spacing = 8;

        for (const auto &cat : m_categories)
        {
            // Indicator
            gc.setColor(cat.color);
            gc.fillRect(item_x, legend_y, indicator_size, indicator_size, 3);

            // Name
            gc.setColor(tm->get_color("window_fg"));
            gc.setDrawFont(theme_font.family.c_str(), theme_font.size, FONT_SLANT_NORMAL,
                           FONT_WEIGHT_BOLD);
            gc.drawText(item_x + indicator_size + text_spacing, legend_y + theme_font.size - 2,
                        cat.name.c_str());

            // Value
            std::string val_str = m_formatter(cat.value);
            gc.setDrawFont(theme_font.family.c_str(), theme_font.size - 1, FONT_SLANT_NORMAL,
                           FONT_WEIGHT_NORMAL);
            gc.setColor(tm->get_color("window_fg").with_alpha(0.7f));
            gc.drawText(item_x + indicator_size + text_spacing,
                        legend_y + (theme_font.size * 2) - 0, val_str.c_str());

            // Calculate width for next item
            auto name_metrics =
                gc.getTextMetrics(cat.name.c_str(), theme_font.family.c_str(), theme_font.size,
                                  FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
            auto val_metrics =
                gc.getTextMetrics(val_str.c_str(), theme_font.family.c_str(), theme_font.size - 1,
                                  FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

            int item_width =
                std::max(name_metrics.width, val_metrics.width) + indicator_size + text_spacing;

            item_x += item_width + item_spacing;

            // Check for wrapping
            if (item_x > m_x + m_width - 20)
            {
                item_x = m_x;
                legend_y += (theme_font.size * 2) + 10;
            }
        }
    }
} // namespace horizon
