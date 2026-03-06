#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    Label::Label() : Widget() {}

    Label::Label(const std::string &text) : Widget(), m_text(text) {}

    int Label::preferred_width() const
    {
        if (m_text.empty())
            return 0;
        if (!application() || !application()->theme_manager)
            return 0;

        auto theme_font = application()->theme_manager->get_font("window");
        int size = (m_font_size > 0) ? m_font_size : theme_font.size;

        unsigned char tmp_buf[4] = {0};
        CairoGraphicContext measure_ctx(tmp_buf, 1, 1);

        auto metrics = measure_ctx.getTextMetrics(m_text.c_str(), theme_font.family.c_str(), size,
                                                  m_font_slant, m_font_weight);
        return static_cast<int>(std::ceil(metrics.width));
    }

    int Label::preferred_height() const
    {
        if (!application() || !application()->theme_manager)
            return 20;

        auto theme_font = application()->theme_manager->get_font("window");
        int size = (m_font_size > 0) ? m_font_size : theme_font.size;
        return size + 4;
    }

    int Label::preferred_height(int width) const
    {
        if (m_text.empty() || width <= 0)
            return 0;

        if (!application() || !application()->theme_manager)
            return 20;

        auto theme_font = application()->theme_manager->get_font("window");
        int size = (m_font_size > 0) ? m_font_size : theme_font.size;
        int line_height = size + 4;

        unsigned char tmp_buf[4] = {0};
        CairoGraphicContext measure_ctx(tmp_buf, 1, 1);

        // We use a large height to calculate all lines
        auto lines =
            const_cast<Label *>(this)->calculate_lines(measure_ctx, width, 100000, line_height);

        if (lines.empty())
            return line_height;

        return (int)lines.size() * line_height;
    }

    void Label::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto theme_font = tm->get_font("window");

        std::string family = theme_font.family;
        int size = (m_font_size > 0) ? m_font_size : theme_font.size;

        Color text_color;
        if (m_text_color.a >= 0)
        {
            text_color = m_text_color;
        }
        else
        {
            text_color = tm->get_color("window_fg");
        }

        gc.setDrawFont(family.c_str(), size, m_font_slant, m_font_weight);
        gc.setColor(text_color);

        int line_height = size + 4;

        // Cache-based layout
        if (m_last_width != m_available_draw_width || m_last_height != m_available_draw_height ||
            m_last_text != m_text || m_last_font_weight != m_font_weight ||
            m_last_font_size != size)
        {
            m_cached_lines =
                calculate_lines(gc, m_available_draw_width, m_available_draw_height, line_height);
            m_last_width = m_available_draw_width;
            m_last_height = m_available_draw_height;
            m_last_text = m_text;
            m_last_font_weight = m_font_weight;
            m_last_font_size = size;
        }

        const auto &lines = m_cached_lines;

        int total_height = 0;
        if (!lines.empty())
        {
            total_height = (int)lines.size() * line_height - 4; // Subtract trailing padding
        }

        int start_y = m_y;

        if (m_vertical_alignment == VerticalAlignment::Middle && m_height > total_height)
        {
            start_y += (m_height - total_height) / 2;
        }
        else if (m_vertical_alignment == VerticalAlignment::Bottom && m_height > total_height)
        {
            start_y += (m_height - total_height);
        }

        for (size_t i = 0; i < lines.size(); ++i)
        {
            int current_y = start_y + (i * line_height) + size - 3;
            int text_x = m_x;

            if (m_alignment == TextAlignment::Center || m_alignment == TextAlignment::Right)
            {
                auto metrics = gc.getTextMetrics(lines[i].c_str(), family.c_str(), size,
                                                 m_font_slant, m_font_weight);
                if (m_alignment == TextAlignment::Center)
                {
                    text_x += (m_width - metrics.width) / 2;
                }
                else
                {
                    text_x += (m_width - metrics.width);
                }
            }

            gc.drawText(text_x, current_y, lines[i].c_str());
        }
    }

    void Label::set_text(const std::string &text)
    {
        m_text = text;
        invalidate();
    }

    const std::string &Label::text() const
    {
        return m_text;
    }

    void Label::set_alignment(TextAlignment alignment)
    {
        m_alignment = alignment;
        invalidate();
    }

    TextAlignment Label::alignment() const
    {
        return m_alignment;
    }

    void Label::set_vertical_alignment(VerticalAlignment alignment)
    {
        m_vertical_alignment = alignment;
        invalidate();
    }

    VerticalAlignment Label::vertical_alignment() const
    {
        return m_vertical_alignment;
    }

    void Label::set_font_weight(FontWeight weight)
    {
        m_font_weight = weight;
        invalidate();
    }

    FontWeight Label::font_weight() const
    {
        return m_font_weight;
    }

    void Label::set_font_slant(FontSlant slant)
    {
        m_font_slant = slant;
        invalidate();
    }

    FontSlant Label::font_slant() const
    {
        return m_font_slant;
    }

    void Label::set_font_size(int size)
    {
        m_font_size = size;
        invalidate();
    }

    void Label::set_text_color(Color color)
    {
        m_text_color = color;
        invalidate();
    }

    std::vector<std::string> Label::calculate_lines(GraphicsContext &gc, int max_width,
                                                    int max_height, int line_height)
    {
        if (m_text.empty() || max_width <= 0)
            return {};

        std::vector<std::string> lines;
        auto theme_font = application()->theme_manager->get_font("window");
        int font_size = (m_font_size > 0) ? m_font_size : theme_font.size;
        const char *font_family = theme_font.family.c_str();

        // Split by hard newlines first
        size_t start = 0;
        size_t end = m_text.find('\n');
        while (true)
        {
            std::string hard_line = m_text.substr(start, end - start);

            // Soft wrap this hard line
            if (hard_line.empty())
            {
                lines.push_back("");
            }
            else
            {
                std::vector<std::string> words;
                size_t w_start = 0;
                size_t w_end = hard_line.find(' ');
                while (true)
                {
                    words.push_back(hard_line.substr(w_start, w_end - w_start));
                    if (w_end == std::string::npos)
                        break;
                    w_start = w_end + 1;
                    w_end = hard_line.find(' ', w_start);
                }

                std::string current_line;
                for (const auto &word : words)
                {
                    if (word.empty())
                        continue;
                    std::string test_line = current_line.empty() ? word : current_line + " " + word;
                    auto metrics = gc.getTextMetrics(test_line.c_str(), font_family, font_size,
                                                     m_font_slant, m_font_weight);

                    if (metrics.width > max_width && !current_line.empty())
                    {
                        lines.push_back(current_line);
                        current_line = word;
                    }
                    else
                    {
                        current_line = test_line;
                    }
                }
                if (!current_line.empty())
                    lines.push_back(current_line);
            }

            if (end == std::string::npos)
                break;
            start = end + 1;
            end = m_text.find('\n', start);
        }

        int available_lines = max_height / line_height;
        if (available_lines <= 0 && max_height > 0)
            available_lines = 1;

        bool height_truncated = false;
        if (available_lines > 0 && lines.size() > (size_t)available_lines)
        {
            lines.resize(available_lines);
            height_truncated = true;
        }

        // Apply ellipsis to lines that exceed width or the last line if height-truncated
        for (size_t i = 0; i < lines.size(); ++i)
        {
            bool is_last = (i == lines.size() - 1);
            auto metrics = gc.getTextMetrics(lines[i].c_str(), font_family, font_size, m_font_slant,
                                             m_font_weight);

            if (metrics.width > max_width || (is_last && height_truncated))
            {
                std::string &text = lines[i];
                text += "...";
                while (text.length() > 3)
                {
                    auto m = gc.getTextMetrics(text.c_str(), font_family, font_size, m_font_slant,
                                               m_font_weight);
                    if (m.width <= max_width)
                        break;
                    // Remove character before ellipsis (the character at pos length-4)
                    text.erase(text.length() - 4, 1);
                }
                if (text == "...")
                {
                    // If even the ellipsis doesn't fit, well...
                    auto m = gc.getTextMetrics("...", font_family, font_size, m_font_slant,
                                               m_font_weight);
                    if (m.width > max_width)
                        text = "";
                }
            }
        }

        return lines;
    }

} // namespace horizon
