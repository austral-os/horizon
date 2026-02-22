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

        virtual void paint() {};
        virtual void setColor(float r, float g, float b, float a = 1.0f) {};
        virtual void setColor(Color color) {};
        virtual void drawRect(int x, int y, int width, int height, CornerRadius radius = 0,
                              float lineWidth = 1.0f) {};
        virtual void fillRect(int x, int y, int width, int height, CornerRadius radius = 0) {};
        virtual void fillLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            bool vertical = true, CornerRadius radius = 0) {};
        virtual void drawLinearGradientRect(int x, int y, int width, int height, Color c1, Color c2,
                                            bool vertical = true, CornerRadius radius = 0,
                                            float lineWidth = 1.0f) {};
        virtual void setDrawFont(const char *font, int size, FontSlant slant, FontWeight weight) {};
        virtual TextMetrics getTextMetrics(const char *text, const char *font, int size,
                                           FontSlant slant, FontWeight weight) const = 0;
        virtual void drawText(int x, int y, const char *text) {};

        virtual void drawCircle(int x, int y, int radius, float lineWidth = 1.0f) {};
        virtual void fillCircle(int x, int y, int radius) {};

        virtual void drawGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                        GradientDirection direction = GradientDirection::Radial,
                                        float lineWidth = 1.0f) {};
        virtual void fillGradientCircle(int x, int y, int radius, Color c1, Color c2,
                                        GradientDirection direction = GradientDirection::Radial) {};

        virtual void flush() {};
    };

} // namespace horizon