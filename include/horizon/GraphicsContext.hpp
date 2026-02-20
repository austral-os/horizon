#pragma once

namespace horizon
{

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void setColor(float r, float g, float b, float a = 1.0f);
        virtual void drawRect(int x, int y, int width, int height);
        virtual void fillRect(int x, int y, int width, int height);
        virtual void flush();
    };

} // namespace horizon