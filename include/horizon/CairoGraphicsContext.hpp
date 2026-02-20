#pragma once

#include <cairo/cairo.h>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{

    class CairoGraphicContext : public GraphicsContext
    {
    public:
        CairoGraphicContext(cairo_surface_t *surface);
        ~CairoGraphicContext() override;

        void setColor(float r, float g, float b, float a = 1.0f) override;
        void drawRect(int x, int y, int width, int height) override;
        void fillRect(int x, int y, int width, int height) override;
        void flush() override;

    private:
        cairo_surface_t *m_surface;
        cairo_t *m_cr;
    };
} // namespace horizon
