#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <horizon/TitlebarCircleButton.hpp>
#include <iostream>

namespace horizon
{
    TitlebarCircleButton::TitlebarCircleButton() : Widget() {}

    TitlebarCircleButton::~TitlebarCircleButton() {}

    void TitlebarCircleButton::draw(GraphicsContext &gc)
    {
        std::cout << "TitlebarCircleButton::draw" << std::endl;
        Color border{0.0f, 0.0f, 0.0f, 1.0f};

        gc.setColor(border);
        int radius = m_width / 2;
        gc.drawCircle(m_start_draw_x + radius, m_start_draw_y + radius, radius, 1.0f);
    }
} // namespace horizon