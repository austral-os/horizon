#include "horizon/Application.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <horizon/TitlebarCircleButton.hpp>

namespace horizon
{
    TitlebarCircleButton::TitlebarCircleButton(Color color) : Widget()
    {
        m_bg_color = color;
    }

    TitlebarCircleButton::~TitlebarCircleButton() {}

    void TitlebarCircleButton::draw(GraphicsContext &gc)
    {
        // std::cout << "TitlebarCircleButton::draw" << std::endl;

        auto *tm = application()->theme_manager.get();
        auto variant = tm->get_variant();

        Color border = variant == "dark" ? tm->get_color("titlebar_border").darker(20.0f)
                                         : tm->get_color("titlebar_border");
        Color border2 = variant == "dark" ? border.lighter(20.0f) : border.lighter(90.0f);

        int radius = m_width / 2;
        int center_x = m_start_draw_x + m_height / 2 - radius;
        int center_y = m_start_draw_y + m_height / 2;

        gc.setColor(border);
        gc.drawCircle(center_x, center_y, radius, 1.0f);

        gc.setColor(border2);
        gc.drawCircle(center_x, center_y, radius + 2, 1.0f);

        gc.setColor(m_bg_color);
        gc.fillCircle(center_x, center_y, radius + 1);

        // gc.fillGradientCircle(center_x, center_y, radius, m_bg_color, {0.0f, 0.0f, 0.0f, 0.5f});
    }

} // namespace horizon