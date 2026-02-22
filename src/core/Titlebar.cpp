#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <horizon/Titlebar.hpp>

namespace horizon
{
    Titlebar::Titlebar(std::string title) : m_title(std::move(title)) {}

    void Titlebar::set_title(std::string title)
    {
        m_title = std::move(title);
    }

    const std::string &Titlebar::title() const
    {
        return m_title;
    }

    void Titlebar::render(GraphicsContext &gc)
    {
        Widget::render(gc);
    }

    void Titlebar::draw(GraphicsContext &gc)
    {

        // Dibujarmos una barra de titulo como la de mac os mountain lion.

        Color black = {0.0f, 0.0f, 0.0f, 1.0f};
        Color brd = {0.47f, 0.47f, 0.47f, 1.0f};
        Color darkGray = {0.847f, 0.847f, 0.847f, 1.0f};
        Color lightGray = {0.98f, 0.98f, 0.98f, 1.0f};

        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                                  m_available_draw_height, lightGray, darkGray, true,
                                  CornerRadius(10, 10, 0, 0));

        gc.setColor(brd);
        gc.drawRect(0, m_height - 1, m_width, 0, 0, 0.8f);

        TextMetrics metrics = gc.getTextMetrics(m_title.c_str(), "Lucida Grande", 16,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);

        int text_x = (m_width / 2) - (metrics.width / 2);
        int text_y = (m_height / 2) + (metrics.height / 2);

        gc.setDrawFont("Lucida Grande", 16, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
        gc.setColor(black);
        gc.drawText(text_x, text_y, m_title.c_str());

        // Dibuja a los hijos...
        Widget::draw(gc);
    }
} // namespace horizon