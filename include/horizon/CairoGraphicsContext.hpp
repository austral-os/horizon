#pragma once

#include <cairo/cairo.h>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{

    class CairoGraphicContext : public GraphicsContext
    {
    public:
        CairoGraphicContext(void *data, int w, int h);
        ~CairoGraphicContext();

        void paint() override;
        void setColor(float r, float g, float b, float a = 1.0f) override;
        void setColor(Color color) override;
        void drawRect(int x, int y, int width, int height, CornerRadius radius = 0,
                      float lineWidth = 1.0f) override;
        void fillRect(int x, int y, int width, int height, CornerRadius radius = 0) override;
        void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                    bool vertical = true, CornerRadius radius = 0) override;
        void drawLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                    bool vertical = true, CornerRadius radius = 0,
                                    float lineWidth = 1.0f) override;
        void flush() override;

    private:
        cairo_surface_t *cairo_s;
        cairo_t *cr;
    };
} // namespace horizon
