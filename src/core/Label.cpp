#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <sstream>

namespace horizon
{
    Label::Label() : Widget() {}

    Label::Label(const std::string &text) : Widget(), m_text(text) {}

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

        auto lines =
            calculate_lines(gc, m_available_draw_width, m_available_draw_height, line_height);

        int total_height = lines.size() * line_height;
        int start_y = m_y;

        if (m_height > total_height)
        {
            start_y += (m_height - total_height) / 2;
        }

        for (size_t i = 0; i < lines.size(); ++i)
        {
            int current_y = start_y + (i * line_height) + size;
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
        std::stringstream ss(m_text);
        std::string line;

        auto theme_font = application()->theme_manager->get_font("window");
        int font_size = (m_font_size > 0) ? m_font_size : theme_font.size;

        while (std::getline(ss, line))
        {
            std::string current_line;
            std::stringstream words(line);
            std::string word;

            while (words >> word)
            {
                std::string test_line = current_line.empty() ? word : current_line + " " + word;
                auto metrics = gc.getTextMetrics(test_line.c_str(), theme_font.family.c_str(),
                                                 font_size, m_font_slant, m_font_weight);

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

        int available_lines = max_height / line_height;
        if (available_lines > 0 && lines.size() > (size_t)available_lines)
        {
            lines.resize(available_lines);
        }

        return lines;
    }

} // namespace horizon
