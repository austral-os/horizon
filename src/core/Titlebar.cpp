#include "horizon/GraphicsContext.hpp"
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

        Color darkGray = {0.847f, 0.847f, 0.847f, 1.0f};
        Color lightGray = {0.98f, 0.98f, 0.98f, 1.0f};

        gc.setColor(darkGray);
        gc.drawRect(0, 0, m_width, m_height, 1);

        gc.fillLinearGradientRect(0, 0, m_width, m_height, lightGray, darkGray, true,
                                  CornerRadius(10, 10, 0, 0));
    }
} // namespace horizon