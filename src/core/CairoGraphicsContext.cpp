#include <horizon/CairoGraphicsContext.hpp>

namespace horizon
{

    CairoGraphicContext::CairoGraphicContext(void *data, int w, int h)
    {
        cairo_surface_t *cairo_s = cairo_image_surface_create_for_data(
            (unsigned char *)data, CAIRO_FORMAT_ARGB32, w, h, w * 4);
        cairo_t *cr = cairo_create(cairo_s);
    }

    CairoGraphicContext::~CairoGraphicContext()
    {
        cairo_destroy(cr);
        cairo_surface_destroy(cairo_s);
    }
    void CairoGraphicContext::setColor(float r, float g, float b, float a)
    {
        cairo_set_source_rgba(cr, r, g, b, a);
    }

    void CairoGraphicContext::drawRect(int x, int y, int width, int height)
    {
        cairo_rectangle(cr, x, y, width, height);
        cairo_stroke(cr);
    }

    void CairoGraphicContext::fillRect(int x, int y, int width, int height)
    {
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
    }

    void CairoGraphicContext::flush()
    {
        cairo_surface_flush(cairo_s);
    }

} // namespace horizon
