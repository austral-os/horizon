#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <horizon/TitlebarCircleButton.hpp>
#include <iostream>

namespace horizon
{
    TitlebarCircleButton::TitlebarCircleButton(Color color) : Widget()
    {
        m_bg_color = color;
    }

    TitlebarCircleButton::~TitlebarCircleButton() {}

    void TitlebarCircleButton::draw(GraphicsContext &gc)
    {
        std::cout << "TitlebarCircleButton::draw" << std::endl;
        Color border{0.4f, 0.4f, 0.4f, 1.0f};

        gc.setColor(border);
        int radius = m_width / 2;
        int center_x = m_start_draw_x + m_height / 2;
        int center_y = m_start_draw_y + m_height / 2;
        gc.drawCircle(center_x, center_y, radius, 1.0f);

        gc.fillGradientCircle(center_x, center_y, radius, m_bg_color, {1.0f, 1.0f, 1.0f, 1.0f});
    }
} // namespace horizon