#pragma once

namespace horizon
{

    struct Color
    {
        float r, g, b, a;
    };

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void paint() {};
        virtual void setColor(float r, float g, float b, float a = 1.0f) {};
        virtual void drawRect(int x, int y, int width, int height) {};
        virtual void fillRect(int x, int y, int width, int height) {};
        virtual void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            bool vertical = true) {};
        virtual void flush() {};
    };

} // namespace horizon