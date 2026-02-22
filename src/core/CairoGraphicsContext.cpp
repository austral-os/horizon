#include <horizon/CairoGraphicsContext.hpp>

namespace horizon
{

    CairoGraphicContext::CairoGraphicContext(void *data, int w, int h)
    {
        cairo_s = cairo_image_surface_create_for_data((unsigned char *)data, CAIRO_FORMAT_ARGB32, w,
                                                      h, w * 4);
        cr = cairo_create(cairo_s);
    }

    CairoGraphicContext::~CairoGraphicContext()
    {
        if (cr)
            cairo_destroy(cr);
        if (cairo_s)
            cairo_surface_destroy(cairo_s);
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

    void CairoGraphicContext::fillLinearGradientRect(int x, int y, int width, int height, Color c1,
                                                     Color c2, bool vertical)
    {
        cairo_pattern_t *pat;
        if (vertical)
        {
            pat = cairo_pattern_create_linear(x, y, x, y + height);
        }
        else
        {
            pat = cairo_pattern_create_linear(x, y, x + width, y);
        }

        cairo_pattern_add_color_stop_rgba(pat, 0, c1.r, c1.g, c1.b, c1.a);
        cairo_pattern_add_color_stop_rgba(pat, 1, c2.r, c2.g, c2.b, c2.a);

        cairo_set_source(cr, pat);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::flush()
    {
        cairo_surface_flush(cairo_s);
    }

} // namespace horizon
