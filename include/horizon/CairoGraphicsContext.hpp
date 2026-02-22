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
        void drawRect(int x, int y, int width, int height, int radius = 0) override;
        void fillRect(int x, int y, int width, int height, int radius = 0) override;
        void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                    bool vertical = true, int radius = 0) override;
        void flush() override;

    private:
        cairo_surface_t *cairo_s;
        cairo_t *cr;
    };
} // namespace horizon
