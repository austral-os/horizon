#include <cmath>
#include <horizon/CairoGraphicsContext.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    void CairoGraphicContext::setColor(Color color)
    {
        cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
    }

    void CairoGraphicContext::paint()
    {
        cairo_paint(cr);
    }

    TextMetrics CairoGraphicContext::getTextMetrics(const char *text, const char *font, int size,
                                                    FontSlant slant, FontWeight weight) const
    {
        TextMetrics metrics;
        cairo_font_slant_t cairo_slant;
        cairo_font_weight_t cairo_weight;
        switch (slant)
        {
        case FONT_SLANT_NORMAL:
            cairo_slant = CAIRO_FONT_SLANT_NORMAL;
            break;
        case FONT_SLANT_ITALIC:
            cairo_slant = CAIRO_FONT_SLANT_ITALIC;
            break;
        case FONT_SLANT_OBLIQUE:
            cairo_slant = CAIRO_FONT_SLANT_OBLIQUE;
            break;
        }
        switch (weight)
        {
        case FONT_WEIGHT_NORMAL:
            cairo_weight = CAIRO_FONT_WEIGHT_NORMAL;
            break;
        case FONT_WEIGHT_BOLD:
            cairo_weight = CAIRO_FONT_WEIGHT_BOLD;
            break;
        }
        cairo_select_font_face(cr, font, cairo_slant, cairo_weight);
        cairo_set_font_size(cr, size);
        cairo_text_extents_t text_extents;
        cairo_text_extents(cr, text, &text_extents);
        metrics.width = text_extents.width;
        metrics.height = text_extents.height;
        return metrics;
    }

    void CairoGraphicContext::setDrawFont(const char *font, int size, FontSlant slant,
                                          FontWeight weight)
    {
        cairo_font_slant_t cairo_slant;
        cairo_font_weight_t cairo_weight;
        switch (slant)
        {
        case FONT_SLANT_NORMAL:
            cairo_slant = CAIRO_FONT_SLANT_NORMAL;
            break;
        case FONT_SLANT_ITALIC:
            cairo_slant = CAIRO_FONT_SLANT_ITALIC;
            break;
        case FONT_SLANT_OBLIQUE:
            cairo_slant = CAIRO_FONT_SLANT_OBLIQUE;
            break;
        }
        switch (weight)
        {
        case FONT_WEIGHT_NORMAL:
            cairo_weight = CAIRO_FONT_WEIGHT_NORMAL;
            break;
        case FONT_WEIGHT_BOLD:
            cairo_weight = CAIRO_FONT_WEIGHT_BOLD;
            break;
        }
        cairo_select_font_face(cr, font, cairo_slant, cairo_weight);
        cairo_set_font_size(cr, size);
    }

    void CairoGraphicContext::drawText(int x, int y, const char *text)
    {
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
    }

    static void rounded_rectangle(cairo_t *cr, double x, double y, double width, double height,
                                  CornerRadius radius)
    {
        if (radius.top_left <= 0 && radius.top_right <= 0 && radius.bottom_right <= 0 &&
            radius.bottom_left <= 0)
        {
            cairo_rectangle(cr, x, y, width, height);
            return;
        }

        double degrees = M_PI / 180.0;

        cairo_new_sub_path(cr);
        // Top right
        cairo_arc(cr, x + width - radius.top_right, y + radius.top_right, radius.top_right,
                  -90 * degrees, 0 * degrees);
        // Bottom right
        cairo_arc(cr, x + width - radius.bottom_right, y + height - radius.bottom_right,
                  radius.bottom_right, 0 * degrees, 90 * degrees);
        // Bottom left
        cairo_arc(cr, x + radius.bottom_left, y + height - radius.bottom_left, radius.bottom_left,
                  90 * degrees, 180 * degrees);
        // Top left
        cairo_arc(cr, x + radius.top_left, y + radius.top_left, radius.top_left, 180 * degrees,
                  270 * degrees);
        cairo_close_path(cr);
    }

    void CairoGraphicContext::drawRect(int x, int y, int width, int height, CornerRadius radius,
                                       float lineWidth)
    {
        cairo_set_line_width(cr, lineWidth);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_stroke(cr);
    }

    void CairoGraphicContext::fillRect(int x, int y, int width, int height, CornerRadius radius)
    {
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_fill(cr);
    }

    void CairoGraphicContext::fillLinearGradientRect(int x, int y, int width, int height, Color c1,
                                                     Color c2, bool vertical, CornerRadius radius)
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

    void CairoGraphicContext::drawLinearGradientRect(int x, int y, int width, int height, Color c1,
                                                     Color c2, bool vertical, CornerRadius radius,
                                                     float lineWidth)
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
        cairo_set_line_width(cr, lineWidth);
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_stroke(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::flush()
    {
        cairo_surface_flush(cairo_s);
    }

} // namespace horizon
