#include <horizon/CairoGraphicsContext.hpp>
#include <stdexcept>

namespace horizon
{

    CairoGraphicContext::CairoGraphicContext(cairo_surface_t *surface)
        : m_surface(surface), m_cr(nullptr)
    {
        if (!m_surface)
        {
            throw std::runtime_error("CairoGraphicContext: surface is null");
        }

        m_cr = cairo_create(m_surface);
        if (!m_cr)
        {
            throw std::runtime_error("CairoGraphicContext: failed to create cairo context");
        }
    }

    CairoGraphicContext::~CairoGraphicContext()
    {
        if (m_cr)
        {
            cairo_destroy(m_cr);
        }
    }

    void CairoGraphicContext::setColor(float r, float g, float b, float a)
    {
        cairo_set_source_rgba(m_cr, r, g, b, a);
    }

    void CairoGraphicContext::drawRect(int x, int y, int width, int height)
    {
        cairo_rectangle(m_cr, x, y, width, height);
        cairo_stroke(m_cr);
    }

    void CairoGraphicContext::fillRect(int x, int y, int width, int height)
    {
        cairo_rectangle(m_cr, x, y, width, height);
        cairo_fill(m_cr);
    }

    void CairoGraphicContext::flush()
    {
        cairo_surface_flush(m_surface);
    }

} // namespace horizon
