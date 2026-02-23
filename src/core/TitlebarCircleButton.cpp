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
        Color border(0.4f, 0.4f, 0.4f, 1.0f);
        Color border2(0.9f, 0.9f, 0.9f, 1.0f);

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