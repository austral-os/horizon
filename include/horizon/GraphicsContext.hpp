#pragma once

#include "horizon/Color.hpp"
#include <memory>
#include <string>
#include <vector>
namespace horizon
{

    struct CornerRadius
    {
        int top_left;
        int top_right;
        int bottom_right;
        int bottom_left;

        CornerRadius(int radius = 0)
            : top_left(radius), top_right(radius), bottom_right(radius), bottom_left(radius)
        {
        }
        CornerRadius(int tl, int tr, int br, int bl)
            : top_left(tl), top_right(tr), bottom_right(br), bottom_left(bl)
        {
        }
    };

    struct PolygonPoint
    {
        int x;
        int y;
        int radius;

        PolygonPoint(int x = 0, int y = 0, int radius = 0) : x(x), y(y), radius(radius) {}
    };

    struct TextMetrics
    {
        int width;
        int height;
    };

    enum FontSlant
    {
        FONT_SLANT_NORMAL = 0,
        FONT_SLANT_ITALIC = 1,
        FONT_SLANT_OBLIQUE = 2
    };

    enum FontWeight
    {
        FONT_WEIGHT_NORMAL = 0,
        FONT_WEIGHT_BOLD = 1
    };

    enum class GradientDirection
    {
        Vertical,
        Horizontal,
        Radial
    };

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;
        virtual int width() const = 0;
        virtual int height() const = 0;

        virtual void paint() {};
        virtual void setColor(float r, float g, float b, float a = 1.0f) {};
        virtual void setColor(Color color) {};
        virtual void clearRect(int x, int y, int width, int height, CornerRadius radius = 0) {};
        virtual void drawRect(int x, int y, int width, int height, CornerRadius radius = 0,
                              float lineWidth = 1.0f) {};
        virtual void fillRect(int x, int y, int width, int height, CornerRadius radius = 0) {};
        virtual void drawLine(int x1, int y1, int x2, int y2, float lineWidth = 1.0f) {};
        virtual void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            bool vertical = true, CornerRadius radius = 0) {};
        virtual void drawLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            float lineWidth = 1.0f, bool vertical = true,
                                            CornerRadius radius = 0) {};
        virtual void save() {};
        virtual void restore() {};
        virtual void clip(int x, int y, int width, int height) {};
        virtual void clipRoundedRect(int x, int y, int width, int height, CornerRadius radius) {};
        virtual void setDrawFont(const char *font, int size, FontSlant slant, FontWeight weight) {};
        virtual TextMetrics getTextMetrics(const char *text, const char *font, int size,
                                           FontSlant slant, FontWeight weight) const = 0;
        virtual void drawText(int x, int y, const char *text) {};

        virtual void drawImage(const std::string &path, int x, int y, int w, int h) {};

        virtual void drawCircle(int x, int y, int radius, float lineWidth = 1.0f) {};
        virtual void fillCircle(int x, int y, int radius) {};

        virtual void drawGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                        GradientDirection direction = GradientDirection::Radial,
                                        float lineWidth = 1.0f) {};
        virtual void fillGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                        GradientDirection direction = GradientDirection::Radial) {};

        virtual void flush() {};

        virtual void pushGroup() {};
        virtual void popGroup() {};
        /**
         * @brief Pops the current Cairo group and uploads it to an OpenGL texture.
         * @param texture_id The ID of the generated texture.
         * @param x The x-offset of the area to capture.
         * @param y The y-offset of the area to capture.
         * @param w The width of the area to capture.
         * @param h The height of the area to capture.
         */
        virtual void popGroupToTexture(uint32_t &texture_id, int x, int y, int w, int h) {};

        virtual void drawTexture3D(uint32_t texture_id, int w, int h, float *matrix_4x4,
                                   float opacity = 1.0f, bool delete_texture = true) {};

        virtual void fillPolygon(const std::vector<PolygonPoint> &points) {};
        virtual void drawPolygon(const std::vector<PolygonPoint> &points, float lineWidth = 1.0f) {
        };
        virtual void fillLinearGradientPolygon(const std::vector<PolygonPoint> &points, Color c1,
                                               Color c2, bool vertical = true) {};
        virtual void clipPolygon(const std::vector<PolygonPoint> &points) {};

        /**
         * @brief Get the native drawing context (e.g. cairo_t*).
         */
        virtual void *getNativeContext() = 0;

        /**
         * @brief Factory method to create an image driver appropriate for this context.
         */
        virtual std::unique_ptr<class ImageDriver> createImageDriver(const std::string &path) = 0;

        /**
         * @brief Draws raw pixels to the context.
         */
        virtual void drawPixels(const unsigned char *data, int img_w, int img_h, int x, int y,
                                int w, int h, int channels = 4) = 0;

        /**
         * @brief Draws an SVG file to the context.
         */
        virtual void drawSvg(const std::string &path, int x, int y, int w, int h) = 0;

        /**
         * @brief Gets the intrinsic size of an SVG file.
         */
        virtual void getSvgSize(const std::string &path, int &w, int &h) = 0;
    };

} // namespace horizon