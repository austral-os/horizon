#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    Button::Button() : Widget() {}

    void Button::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto variant = tm->get_variant();

        Color bg_color = tm->get_color("window_bg1").darker(50.0f);
        Color border = variant == "dark" ? tm->get_color("titlebar_border").darker(20.0f)
                                         : tm->get_color("titlebar_border");
        Color border2 = variant == "dark" ? border.lighter(20.0f) : border.lighter(90.0f);

        int radius = 24;

        gc.setColor(border);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height,
                    {radius, radius, radius, radius});

        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_width, m_height,
                                  bg_color.lighter(80.0f), bg_color.lighter(50.0f), true,
                                  {radius, radius, radius, radius});
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