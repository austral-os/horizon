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

        void setColor(float r, float g, float b, float a = 1.0f);
        void drawRect(int x, int y, int width, int height);
        void fillRect(int x, int y, int width, int height);
        void flush();

    private:
        cairo_surface_t *cairo_s;
        cairo_t *cr;
    };
} // namespace horizon
