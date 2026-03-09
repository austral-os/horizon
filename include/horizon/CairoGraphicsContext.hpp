#pragma once

#include <cairo/cairo.h>
#include <horizon/GraphicsContext.hpp>
#include <map>

namespace horizon
{

    class Application;

    class CairoGraphicContext : public GraphicsContext
    {
    public:
        CairoGraphicContext(const Application *app, void *data, int w, int h);
        ~CairoGraphicContext();

        void paint() override;
        void setColor(float r, float g, float b, float a = 1.0f) override;
        void setColor(Color color) override;
        void clearRect(int x, int y, int width, int height, CornerRadius radius = 0) override;
        void drawRect(int x, int y, int width, int height, CornerRadius radius = 0,
                      float lineWidth = 1.0f) override;
        void fillRect(int x, int y, int width, int height, CornerRadius radius = 0) override;
        void drawLine(int x1, int y1, int x2, int y2, float lineWidth = 1.0f) override;
        void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                    bool vertical = true, CornerRadius radius = 0) override;
        void drawLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                    float lineWidth = 1.0f, bool vertical = true,
                                    CornerRadius radius = 0) override;

        void setDrawFont(const char *font, int size, FontSlant slant, FontWeight weight) override;
        TextMetrics getTextMetrics(const char *text, const char *font, int size, FontSlant slant,
                                   FontWeight weight) const override;
        void drawText(int x, int y, const char *text) override;

        void drawImage(const std::string &path, int x, int y, int w, int h) override;

        void drawCircle(int x, int y, int radius, float lineWidth = 1.0f) override;
        void fillCircle(int x, int y, int radius) override;

        void drawGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                GradientDirection direction = GradientDirection::Radial,
                                float lineWidth = 1.0f) override;
        void fillGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                GradientDirection direction = GradientDirection::Radial) override;

        void flush() override;

        void save() override;
        void restore() override;
        void clip(int x, int y, int width, int height) override;
        void clipRoundedRect(int x, int y, int width, int height, CornerRadius radius) override;

        void pushGroup() override;
        void popGroup() override;
        void popGroupToTexture(uint32_t &texture_id, int x, int y, int w, int h) override;

        void drawTexture3D(uint32_t texture_id, int w, int h, float *matrix_4x4,
                           float opacity = 1.0f, bool delete_texture = true) override;

        void fillPolygon(const std::vector<PolygonPoint> &points) override;
        void drawPolygon(const std::vector<PolygonPoint> &points, float lineWidth = 1.0f) override;
        void fillLinearGradientPolygon(const std::vector<PolygonPoint> &points, Color c1, Color c2,
                                       bool vertical = true) override;
        void clipPolygon(const std::vector<PolygonPoint> &points) override;

        void *getNativeContext() override
        {
            return cr;
        }

        std::unique_ptr<ImageDriver> createImageDriver(const std::string &path) override;

        void drawPixels(const unsigned char *data, int img_w, int img_h, int x, int y, int w, int h,
                        int channels = 4) override;
        void drawSvg(const std::string &path, int x, int y, int w, int h) override;
        void getSvgSize(const std::string &path, int &w, int &h) override;

    private:
        const Application *m_app;
        cairo_surface_t *cairo_s;
        cairo_t *cr;

        // Simple SVG handle cache to avoid reparsing every frame
        std::map<std::string, void *> m_svg_cache;
        void *get_svg_handle(const std::string &path);
    };
} // namespace horizon
