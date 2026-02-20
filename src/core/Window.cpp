#include <horizon/GraphicsContext.hpp>
#include <horizon/Window.hpp>

namespace horizon
{

    Window::Window(std::string title) : m_title(std::move(title)) {}

    void Window::setSize(int width, int height)
    {
        m_width = width;
        m_height = height;
    }

    const std::string &Window::title() const
    {
        return m_title;
    }

    void Window::render(GraphicsContext &gc)
    {
        Widget::render(gc);
    }

    void Window::draw(GraphicsContext &gc)
    {
        // Dibujar fondo de ventana
        gc.drawRect(0, 0, m_width, m_height);
    }

} // namespace horizon