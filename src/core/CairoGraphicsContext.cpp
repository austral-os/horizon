#include "horizon/WaylandWindow.hpp"
#include <GLES2/gl2.h>
#include <algorithm>
#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/StbImageDriver.hpp>
#include <horizon/SvgImageDriver.hpp>
#include <librsvg/rsvg.h>
#include <memory>
#include <string>

namespace horizon
{

    CairoGraphicContext::CairoGraphicContext(const WaylandWindow *app, void *data, int w, int h)
        : m_app(app), cairo_s(nullptr), cr(nullptr), m_width(w), m_height(h)
    {
        if (!data || w <= 0 || h <= 0) {
             return;
        }
        cairo_s = cairo_image_surface_create_for_data((unsigned char *)data, CAIRO_FORMAT_ARGB32, w,
                                                      h, w * 4);
        if (!cairo_s || cairo_surface_status(cairo_s) != CAIRO_STATUS_SUCCESS) {
             return;
        }
        cr = cairo_create(cairo_s);
        m_clip_stack.push_back({0, 0, w, h});
    }

    CairoGraphicContext::~CairoGraphicContext()
    {
        if (cr)
            cairo_destroy(cr);
        if (cairo_s)
            cairo_surface_destroy(cairo_s);
    }

    static void rounded_rectangle(cairo_t *cr, double x, double y, double width, double height,
                                  CornerRadius radius)
    {
        if (!cr || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height))
            return;
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

    static void rounded_polygon_path(cairo_t *cr, const std::vector<PolygonPoint> &points)
    {
        if (points.empty())
            return;
        if (points.size() == 1)
        {
            cairo_move_to(cr, points[0].x, points[0].y);
            return;
        }

        cairo_new_path(cr);
        size_t n = points.size();

        for (size_t i = 0; i < n; ++i)
        {
            const auto &p1 = points[(i + n - 1) % n];
            const auto &p2 = points[i];
            const auto &p3 = points[(i + 1) % n];

            if (p2.radius <= 0)
            {
                if (i == 0)
                    cairo_move_to(cr, p2.x, p2.y);
                else
                    cairo_line_to(cr, p2.x, p2.y);
                continue;
            }

            double dx1 = p1.x - p2.x;
            double dy1 = p1.y - p2.y;
            double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);

            double dx2 = p3.x - p2.x;
            double dy2 = p3.y - p2.y;
            double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

            if (len1 < 0.001 || len2 < 0.001)
            {
                if (i == 0)
                    cairo_move_to(cr, p2.x, p2.y);
                else
                    cairo_line_to(cr, p2.x, p2.y);
                continue;
            }

            dx1 /= len1;
            dy1 /= len1;
            dx2 /= len2;
            dy2 /= len2;

            double dot = dx1 * dx2 + dy1 * dy2;
            double cross = dx1 * dy2 - dy1 * dx2;
            double angle = std::acos(std::max(-1.0, std::min(1.0, dot)));

            // If angle is 0 or PI, it's a flat line or a fold-back
            if (angle < 0.001 || angle > M_PI - 0.001)
            {
                if (i == 0)
                    cairo_move_to(cr, p2.x, p2.y);
                else
                    cairo_line_to(cr, p2.x, p2.y);
                continue;
            }

            double tan_len = p2.radius / std::tan(angle / 2.0);
            tan_len = std::min(tan_len, std::min(len1, len2) / 2.0);
            double effective_radius = tan_len * std::tan(angle / 2.0);

            double tx1 = p2.x + dx1 * tan_len;
            double ty1 = p2.y + dy1 * tan_len;
            double tx2 = p2.x + dx2 * tan_len;
            double ty2 = p2.y + dy2 * tan_len;

            if (i == 0)
                cairo_move_to(cr, tx1, ty1);
            else
                cairo_line_to(cr, tx1, ty1);

            double bx = dx1 + dx2;
            double by = dy1 + dy2;
            double blen = std::sqrt(bx * bx + by * by);

            if (blen < 0.001)
            {
                cairo_line_to(cr, p2.x, p2.y);
            }
            else
            {
                bx /= blen;
                by /= blen;
                double dist_to_center = effective_radius / std::sin(angle / 2.0);
                double cx = p2.x + bx * dist_to_center;
                double cy = p2.y + by * dist_to_center;

                double start_angle = std::atan2(ty1 - cy, tx1 - cx);
                double end_angle = std::atan2(ty2 - cy, tx2 - cx);

                // Reversed the condition: cross < 0 -> CW turn -> cairo_arc
                if (cross < 0)
                    cairo_arc(cr, cx, cy, effective_radius, start_angle, end_angle);
                else
                    cairo_arc_negative(cr, cx, cy, effective_radius, start_angle, end_angle);
            }
        }
        cairo_close_path(cr);
    }

    void CairoGraphicContext::setColor(float r, float g, float b, float a)
    {
        if (cr)
        {
            cairo_set_source_rgba(cr, r, g, b, a);
        }
    }

    void CairoGraphicContext::setColor(Color color)
    {
        if (cr)
        {
            cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
        }
    }

    void CairoGraphicContext::clearRect(int x, int y, int w, int h, CornerRadius radius)
    {
        if (!cr) return;
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        rounded_rectangle(cr, x, y, (double)w, (double)h, radius);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    void CairoGraphicContext::paint()
    {
        if (cr)
        {
            cairo_paint(cr);
        }
    }

    TextMetrics CairoGraphicContext::getTextMetrics(const char *text, const char *font, int size,
                                                    FontSlant slant, FontWeight weight) const
    {
        if (!text || !cr) return {0, 0};
        
        TextMetrics metrics;
        // ... (rest of the function)
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
        
        metrics.width = text_extents.x_advance;
        metrics.height = text_extents.height;
        return metrics;
    }

    void CairoGraphicContext::setDrawFont(const char *font, int size, FontSlant slant,
                                          FontWeight weight)
    {
        if (!cr) return;
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
        if (!cr || !text)
            return;
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
    }

    void CairoGraphicContext::drawImage(const std::string &path, int x, int y, int w, int h)
    {
        if (path.empty() || w <= 0 || h <= 0 || !cr)
            return;

        cairo_save(cr);
        // ... (rest of the function remains the same, but using cr safely)
        
        // Determine file type by extension
        bool is_svg = false;
        auto dot_pos = path.rfind('.');
        if (dot_pos != std::string::npos)
        {
            std::string ext = path.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            is_svg = (ext == "svg");
        }

        if (is_svg)
        {
            // --- SVG rendering with surface cache ---
            std::string cache_key = path + "@" + std::to_string(w) + "x" + std::to_string(h);
            cairo_surface_t *img = nullptr;

            if (m_app && m_app->m_surface_cache.count(cache_key))
            {
                img = static_cast<cairo_surface_t *>(m_app->m_surface_cache[cache_key]);
            }
            else
            {
                // Render SVG to a new image surface
                img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
                cairo_t *temp_cr = cairo_create(img);

                void *handle = get_svg_handle(path);
                if (handle)
                {
                    RsvgHandle *rsvg = static_cast<RsvgHandle *>(handle);
                    RsvgRectangle viewport = {0, 0, static_cast<double>(w), static_cast<double>(h)};
                    rsvg_handle_render_document(rsvg, temp_cr, &viewport, nullptr);
                }

                cairo_destroy(temp_cr);

                if (m_app)
                {
                    m_app->m_surface_cache[cache_key] = img;
                }
            }

            if (img)
            {
                cairo_set_source_surface(cr, img, x, y);
                cairo_paint(cr);
            }

            if (!m_app && img)
            {
                cairo_surface_destroy(img);
            }
        }
        else
        {
            // --- PNG rendering via cairo ---
            cairo_surface_t *img = nullptr;
            if (m_app && m_app->m_surface_cache.count(path))
            {
                img = static_cast<cairo_surface_t *>(m_app->m_surface_cache[path]);
            }
            else
            {
                img = cairo_image_surface_create_from_png(path.c_str());
                if (cairo_surface_status(img) != CAIRO_STATUS_SUCCESS)
                {
                    cairo_surface_destroy(img);
                    cairo_restore(cr);
                    return;
                }
                if (m_app)
                {
                    m_app->m_surface_cache[path] = img;
                }
            }

            int img_w = cairo_image_surface_get_width(img);
            int img_h = cairo_image_surface_get_height(img);

            if (img_w > 0 && img_h > 0)
            {
                double sx = static_cast<double>(w) / img_w;
                double sy = static_cast<double>(h) / img_h;

                cairo_translate(cr, x, y);
                cairo_scale(cr, sx, sy);
                cairo_set_source_surface(cr, img, 0, 0);
                cairo_paint(cr);
            }

            // Note: We don't destroy the surface here because it's cached in m_app
            // If m_app is null (shouldn't happen), we'd need to destroy it, but
            // the cache persists for the life of the app.
            if (!m_app && img)
            {
                cairo_surface_destroy(img);
            }
        }

        cairo_restore(cr);
    }

    void CairoGraphicContext::drawRect(int x, int y, int width, int height, CornerRadius radius,
                                       float lineWidth)
    {
        if (!cr) return;
        cairo_set_line_width(cr, lineWidth);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_stroke(cr);
    }

    void CairoGraphicContext::fillRect(int x, int y, int width, int height, CornerRadius radius)
    {
        if (!cr)
            return;

        rounded_rectangle(cr, x, y, width, height, radius);
        cairo_fill(cr);
    }

    void CairoGraphicContext::drawLine(int x1, int y1, int x2, int y2, float lineWidth)
    {
        if (!cr)
            return;
        cairo_save(cr);
        cairo_set_line_width(cr, (double)lineWidth);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, (double)x1, (double)y1);
        cairo_line_to(cr, (double)x2, (double)y2);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    void CairoGraphicContext::fillLinearGradientRect(int x, int y, int width, int height, Color c1,
                                                     Color c2, bool vertical, CornerRadius radius)
    {
        if (!cr)
            return;
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
                                                     Color c2, float lineWidth, bool vertical,
                                                     CornerRadius radius)
    {
        if (!cr)
            return;
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

    static cairo_pattern_t *create_circle_gradient(int x, int y, int radius, Color c1, Color c2,
                                                   GradientDirection direction)
    {
        cairo_pattern_t *pat;
        switch (direction)
        {
        case GradientDirection::Vertical:
            pat = cairo_pattern_create_linear(x, y - radius, x, y + radius);
            break;
        case GradientDirection::Horizontal:
            pat = cairo_pattern_create_linear(x - radius, y, x + radius, y);
            break;
        case GradientDirection::Radial:
        default:
            pat = cairo_pattern_create_radial(x, y, 0, x, y, radius);
            break;
        }

        cairo_pattern_add_color_stop_rgba(pat, 0, c1.r, c1.g, c1.b, c1.a);
        cairo_pattern_add_color_stop_rgba(pat, 1, c2.r, c2.g, c2.b, c2.a);

        return pat;
    }

    void CairoGraphicContext::drawGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                                 GradientDirection direction, float lineWidth)
    {
        if (!cr)
            return;
        cairo_pattern_t *pat = create_circle_gradient(x, y, radius, c1, c2, direction);
        cairo_set_source(cr, pat);
        cairo_set_line_width(cr, lineWidth);
        cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
        cairo_stroke(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::fillGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                                 GradientDirection direction)
    {
        if (!cr)
            return;
        cairo_pattern_t *pat = create_circle_gradient(x, y, radius, c1, c2, direction);
        cairo_set_source(cr, pat);
        cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::drawCircle(int x, int y, int radius, float lineWidth)
    {
        if (!cr)
            return;
        cairo_new_path(cr); // ← levanta el lápiz
        cairo_set_line_width(cr, lineWidth);
        cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
        cairo_stroke(cr); // limpia el path
    }

    void CairoGraphicContext::fillCircle(int x, int y, int radius)
    {
        if (!cr)
            return;
        cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    void CairoGraphicContext::flush()
    {
        if (cairo_s)
            cairo_surface_flush(cairo_s);
    }

    void CairoGraphicContext::save()
    {
        if (cr)
            cairo_save(cr);
        if (!m_clip_stack.empty())
            m_clip_stack.push_back(m_clip_stack.back());
    }

    void CairoGraphicContext::restore()
    {
        if (cr)
            cairo_restore(cr);
        if (!m_clip_stack.empty())
            m_clip_stack.pop_back();
    }

    void CairoGraphicContext::clip(int x, int y, int width, int height)
    {
        if (!cr)
            return;
        cairo_rectangle(cr, x, y, width, height);
        cairo_clip(cr);

        if (!m_clip_stack.empty())
        {
            auto &cur = m_clip_stack.back();
            int nx = std::max(cur.x, x);
            int ny = std::max(cur.y, y);
            int nr = std::min(cur.x + cur.w, x + width);
            int nb = std::min(cur.y + cur.h, y + height);
            cur.x = nx;
            cur.y = ny;
            cur.w = std::max(0, nr - nx);
            cur.h = std::max(0, nb - ny);
        }
    }

    void CairoGraphicContext::clipRoundedRect(int x, int y, int width, int height, CornerRadius radius)
    {
        if (!cr) return;
        rounded_rectangle(cr, (double)x, (double)y, (double)width, (double)height, radius);
        cairo_clip(cr);
    }

    void CairoGraphicContext::translate(float dx, float dy)
    {
        if (cr)
            cairo_translate(cr, dx, dy);
    }

    void CairoGraphicContext::scale(float sx, float sy)
    {
        if (cr)
            cairo_scale(cr, sx, sy);
    }

    void CairoGraphicContext::pushGroup()
    {
        if (cr)
            cairo_push_group(cr);
    }

    void CairoGraphicContext::popGroup()
    {
        if (cr)
        {
            cairo_pop_group_to_source(cr);
            cairo_paint(cr);
        }
    }

    void CairoGraphicContext::popGroupToSource()
    {
        if (cr)
            cairo_pop_group_to_source(cr);
    }

    void CairoGraphicContext::fillPolygon(const std::vector<PolygonPoint> &points)
    {
        if (!cr || points.empty())
            return;
        rounded_polygon_path(cr, points);
        cairo_fill(cr);
    }

    void CairoGraphicContext::drawPolygon(const std::vector<PolygonPoint> &points, float lineWidth)
    {
        if (!cr || points.empty())
            return;
        cairo_set_line_width(cr, lineWidth);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        rounded_polygon_path(cr, points);
        cairo_stroke(cr);
    }

    void CairoGraphicContext::fillLinearGradientPolygon(const std::vector<PolygonPoint> &points,
                                                        Color c1, Color c2, bool vertical)
    {
        if (points.empty() || !cr)
            return;

        // ...
        int min_x = points[0].x, max_x = points[0].x;
        int min_y = points[0].y, max_y = points[0].y;
        for (const auto &p : points)
        {
            min_x = std::min(min_x, p.x);
            max_x = std::max(max_x, p.x);
            min_y = std::min(min_y, p.y);
            max_y = std::max(max_y, p.y);
        }

        cairo_pattern_t *pat;
        if (vertical)
        {
            pat = cairo_pattern_create_linear(min_x, min_y, min_x, max_y);
        }
        else
        {
            pat = cairo_pattern_create_linear(min_x, min_y, max_x, min_y);
        }

        cairo_pattern_add_color_stop_rgba(pat, 0, c1.r, c1.g, c1.b, c1.a);
        cairo_pattern_add_color_stop_rgba(pat, 1, c2.r, c2.g, c2.b, c2.a);

        cairo_set_source(cr, pat);
        rounded_polygon_path(cr, points);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }

    void CairoGraphicContext::clipPolygon(const std::vector<PolygonPoint> &points)
    {
        if (!cr || points.empty())
            return;
        rounded_polygon_path(cr, points);
        cairo_clip(cr);
    }

    void CairoGraphicContext::maskLinearGradient(int x, int y, int w, int h, Color c1, Color c2,
                                                 bool vertical)
    {
        if (!cr) return;
        cairo_pattern_t *mask = vertical ? cairo_pattern_create_linear(x, y, x, y + h)
                                         : cairo_pattern_create_linear(x, y, x + w, y);
        cairo_pattern_add_color_stop_rgba(mask, 0, c1.r, c1.g, c1.b, c1.a);
        cairo_pattern_add_color_stop_rgba(mask, 1, c2.r, c2.g, c2.b, c2.a);
        cairo_mask(cr, mask);
        cairo_pattern_destroy(mask);
    }

    std::unique_ptr<ImageDriver> CairoGraphicContext::createImageDriver(const std::string &path)
    {
        auto dot_pos = path.rfind('.');
        std::string ext = "";
        if (dot_pos != std::string::npos)
        {
            ext = path.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        }

        std::unique_ptr<ImageDriver> driver;
        if (ext == "svg")
        {
            driver = std::make_unique<SvgImageDriver>();
        }
        else
        {
            driver = std::make_unique<StbImageDriver>();
        }

        if (driver && driver->load(path))
        {
            return driver;
        }

        return nullptr;
    }

    void CairoGraphicContext::drawPixels(const unsigned char *data, int img_w, int img_h, int x,
                                         int y, int w, int h, int channels)
    {
        if (!data || img_w <= 0 || img_h <= 0)
            return;

        // Note: Cairo prefers pre-multiplied Alpha, but for STB we'll assume ARGB32 for now
        // or just draw it directly if possible. STB_load(..., 4) gives RGBA.
        // We'll create a temporary surface for the draw.
        // A better way would be for the ImageDriver to hold the surface,
        // but we want the DRIVER to be Cairo-free.
        // Caching the surface in the context based on the data pointer is one option.

        cairo_surface_t *pixel_s = cairo_image_surface_create_for_data(
            const_cast<unsigned char *>(data), CAIRO_FORMAT_ARGB32, img_w, img_h, img_w * 4);

        if (cairo_surface_status(pixel_s) == CAIRO_STATUS_SUCCESS)
        {
            cairo_save(cr);
            double sx = static_cast<double>(w) / img_w;
            double sy = static_cast<double>(h) / img_h;

            cairo_translate(cr, x, y);
            cairo_scale(cr, sx, sy);
            cairo_set_source_surface(cr, pixel_s, 0, 0);
            cairo_paint(cr);
            cairo_restore(cr);
        }

        cairo_surface_destroy(pixel_s);
    }

    void *CairoGraphicContext::get_svg_handle(const std::string &path)
    {
        if (!m_app)
            return nullptr;

        if (m_app->m_svg_cache.count(path))
            return m_app->m_svg_cache[path];

        GError *error = nullptr;
        GFile *gfile = g_file_new_for_path(path.c_str());
        RsvgHandle *handle =
            rsvg_handle_new_from_gfile_sync(gfile, RSVG_HANDLE_FLAGS_NONE, nullptr, &error);
        g_object_unref(gfile);

        if (handle)
        {
            m_app->m_svg_cache[path] = handle;
        }
        else if (error)
        {
            g_error_free(error);
        }

        return handle;
    }

    void CairoGraphicContext::drawSvg(const std::string &path, int x, int y, int w, int h)
    {
        if (!cr) return;
        RsvgHandle *handle = static_cast<RsvgHandle *>(get_svg_handle(path));
        if (!handle)
            return;

        RsvgRectangle viewport = {static_cast<double>(x), static_cast<double>(y),
                                  static_cast<double>(w), static_cast<double>(h)};

        rsvg_handle_render_document(handle, cr, &viewport, nullptr);
    }

    void CairoGraphicContext::getSvgSize(const std::string &path, int &w, int &h)
    {
        RsvgHandle *handle = static_cast<RsvgHandle *>(get_svg_handle(path));
        if (handle)
        {
            double width, height;
            rsvg_handle_get_intrinsic_size_in_pixels(handle, &width, &height);
            w = static_cast<int>(width);
            h = static_cast<int>(height);
        }
        else
        {
            w = h = 0;
        }
    }

    void CairoGraphicContext::popGroupToTexture(uint32_t &texture_id, int x, int y, int w, int h)
    {
        if (!cr || w <= 0 || h <= 0)
            return;

        cairo_pattern_t *group = cairo_pop_group(cr);
        if (!group)
            return;

        // Use a temporary surface to extract the region at (x, y)
        cairo_surface_t *tmp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_t *tmp_cr = cairo_create(tmp_surf);

        // Blit the group content from (x, y) to (0, 0)
        cairo_set_source(tmp_cr, group);
        cairo_matrix_t matrix;
        cairo_matrix_init_translate(&matrix, x, y);
        cairo_pattern_set_matrix(group, &matrix);
        cairo_paint(tmp_cr);

        int surf_w = cairo_image_surface_get_width(tmp_surf);
        int surf_h = cairo_image_surface_get_height(tmp_surf);
        unsigned char *data = cairo_image_surface_get_data(tmp_surf);

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf_w, surf_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        cairo_destroy(tmp_cr);
        cairo_surface_destroy(tmp_surf);
        cairo_pattern_destroy(group);
    }

    void CairoGraphicContext::drawTexture3D(uint32_t texture_id, int w, int h, float *matrix_4x4,
                                            float opacity, bool delete_texture)
    {
        if (!m_app)
            return;

        WaylandWindow::GLDrawCall call;
        call.texture_id = texture_id;
        std::memcpy(call.mvp, matrix_4x4, 16 * sizeof(float));
        call.opacity = opacity;
        call.delete_texture = delete_texture;
        call.gradient_start = 1.0f;
        call.gradient_end = 1.0f;

        if (!m_clip_stack.empty())
        {
            const auto &clip = m_clip_stack.back();
            call.use_scissor = true;
            call.scissor_x = clip.x;
            call.scissor_y = clip.y;
            call.scissor_w = clip.w;
            call.scissor_h = clip.h;
        }
        else
        {
            call.use_scissor = false;
        }

        m_app->queue_gl_draw(call);
    }

    void CairoGraphicContext::deleteTexture(uint32_t texture_id)
    {
        if (texture_id != 0)
            glDeleteTextures(1, &texture_id);
    }
} // namespace horizon
