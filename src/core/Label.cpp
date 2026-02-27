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

        int line_height = size + 4; // Add some spacing between lines
        std::vector<std::string> lines = calculate_lines(gc, m_width, m_height, line_height);

        int current_y = m_y + size; // Initial Y (baseline)

        for (const auto &line : lines)
        {
            TextMetrics metrics =
                gc.getTextMetrics(line.c_str(), family.c_str(), size, m_font_slant, m_font_weight);

            int draw_x = m_x;
            if (m_alignment == TextAlignment::Center)
            {
                draw_x = m_x + (m_width / 2) - (metrics.width / 2);
            }
            else if (m_alignment == TextAlignment::Right)
            {
                draw_x = m_x + m_width - metrics.width;
            }

            gc.drawText(draw_x, current_y, line.c_str());
            current_y += line_height;
        }
    }

    std::vector<std::string> Label::calculate_lines(GraphicsContext &gc, int max_width,
                                                    int max_height, int line_height)
    {
        auto *tm = application()->theme_manager.get();
        auto theme_font = tm->get_font("window");
        std::string family = theme_font.family;
        int size = (m_font_size > 0) ? m_font_size : theme_font.size;

        std::vector<std::string> result;
        std::stringstream ss(m_text);
        std::string word;
        std::string current_line;

        int available_lines = max_height / line_height;
        if (available_lines <= 0)
            return {};

        while (ss >> word)
        {
            std::string test_line = current_line.empty() ? word : current_line + " " + word;
            TextMetrics metrics = gc.getTextMetrics(test_line.c_str(), family.c_str(), size,
                                                    m_font_slant, m_font_weight);

            if (metrics.width > max_width && !current_line.empty())
            {
                if (result.size() + 1 >= (size_t)available_lines)
                {
                    // This was the last available line, and we still have more words.
                    // We need to add ellipsis to current_line.
                    std::string ellipsis = "...";
                    while (!current_line.empty())
                    {
                        std::string truncated = current_line + ellipsis;
                        TextMetrics tm_trunc = gc.getTextMetrics(truncated.c_str(), family.c_str(),
                                                                 size, m_font_slant, m_font_weight);
                        if (tm_trunc.width <= max_width)
                        {
                            result.push_back(truncated);
                            return result;
                        }
                        // Remove last word/char and try again
                        size_t last_space = current_line.find_last_of(" ");
                        if (last_space != std::string::npos)
                        {
                            current_line = current_line.substr(0, last_space);
                        }
                        else
                        {
                            current_line = "";
                        }
                    }
                    result.push_back(ellipsis);
                    return result;
                }

                result.push_back(current_line);
                current_line = word;
            }
            else
            {
                current_line = test_line;
            }
        }

        if (!current_line.empty())
        {
            result.push_back(current_line);
        }

        return result;
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

} // namespace horizon
