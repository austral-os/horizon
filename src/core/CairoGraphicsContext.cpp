#include <cmath>
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

    static void rounded_rectangle(cairo_t *cr, double x, double y, double width, double height,
                                  int radius)
    {
        if (radius <= 0)
        {
            cairo_rectangle(cr, x, y, width, height);
            return;
        }

        double r = static_cast<double>(radius);
        double degrees = M_PI / 180.0;

        cairo_new_sub_path(cr);
        cairo_arc(cr, x + width - r, y + r, r, -90 * degrees, 0 * degrees);
        cairo_arc(cr, x + width - r, y + height - r, r, 0 * degrees, 90 * degrees);
        cairo_arc(cr, x + r, y + height - r, r, 90 * degrees, 180 * degrees);
        cairo_arc(cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
        cairo_close_path(cr);
    }

    void CairoGraphicContext::drawRect(int x, int y, int width, int height, int radius)
    {
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_stroke(cr);
    }

    void CairoGraphicContext::fillRect(int x, int y, int width, int height, int radius)
    {
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_fill(cr);
    }

    void CairoGraphicContext::fillLinearGradientRect(int x, int y, int width, int height, Color c1,
                                                     Color c2, bool vertical, int radius)
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
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::flush()
    {
        cairo_surface_flush(cairo_s);
    }

} // namespace horizon
