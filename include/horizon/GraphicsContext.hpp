#pragma once

namespace horizon
{

    struct Color
    {
        float r, g, b, a;
    };

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

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void paint() {};
        virtual void setColor(float r, float g, float b, float a = 1.0f) {};
        virtual void setColor(Color color) {};
        virtual void drawRect(int x, int y, int width, int height, CornerRadius radius = 0) {};
        virtual void fillRect(int x, int y, int width, int height, CornerRadius radius = 0) {};
        virtual void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            bool vertical = true, CornerRadius radius = 0) {};
        virtual void flush() {};
    };

} // namespace horizon