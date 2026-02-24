#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    Button::Button() : AquaObject()
    {
        when_mouse_enter.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                invalidate();
            });
        when_mouse_leave.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                invalidate();
            });
    }

    void Button::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color window_bg = tm->get_color("window_bg");
        Color window_fg = tm->get_color("window_fg");

        Color text_color = window_fg;

        AquaObject::draw(gc);

        // Center the text
        TextMetrics metrics = gc.getTextMetrics(m_text.c_str(), font.family.c_str(), font.size,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int text_x = m_start_draw_x + (m_width / 2) - (metrics.width / 2);
        int text_y = m_start_draw_y + (m_height / 2) + (metrics.height / 2) - 3;

        // Draw the text
        gc.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        gc.setColor(text_color);
        gc.drawText(text_x, text_y, m_text.c_str());
    }

    void Button::set_text(std::string text)
    {
        m_text = std::move(text);
    }

    const std::string &Button::text() const
    {
        return m_text;
    }

} // namespace horizon