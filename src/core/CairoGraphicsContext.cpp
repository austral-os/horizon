#include <horizon/CairoGraphicsContext.hpp>
#include <iostream>

namespace horizon
{

    CairoGraphicContext::CairoGraphicContext(void *data, int w, int h)
    {
        cairo_s = cairo_image_surface_create_for_data(
            (unsigned char *)data, CAIRO_FORMAT_ARGB32, w, h, w * 4);
        cr = cairo_create(cairo_s);
    }

    CairoGraphicContext::~CairoGraphicContext()
    {
        std::cout << "haciendo free" << std::endl;
        if (cr)
            cairo_destroy(cr);
        if (cairo_s)
            cairo_surface_destroy(cairo_s);
        std::cout << "terminando free" << std::endl;
    }
    void CairoGraphicContext::setColor(float r, float g, float b, float a)
    {
        cairo_set_source_rgba(cr, r, g, b, a);
    }

    void CairoGraphicContext::paint()
    {
        cairo_paint(cr);
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
