#include <horizon/Application.hpp>
#include <horizon/Frame.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{

    Frame::Frame() {}

    void Frame::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();

        int radius = 10;

        Color bg = tm->get_color("window_bg").darker(50.f).with_alpha(0.08f);

        Color border = tm->get_color("window_border").lighter(40.f);

        gc.setColor(bg.lighter(100.f));
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height,
                    {radius});

        gc.setColor(bg);
        gc.fillRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                    m_available_draw_height - 2, {radius});

        gc.setColor(border);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                    m_available_draw_height - 2, {radius});

        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                                  radius * 0.8f,
                                  border.with_alpha(0.35f), // Blanco sólido arriba
                                  border.with_alpha(0.0f),  // Blanco casi transparente abajo
                                  true, {radius, radius, 0, 0});
    }

} // namespace horizon