#pragma once

#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    class Panel : public Widget
    {
    public:
        Panel();
        ~Panel() = default;

        void set_background_colors(Color bg1, Color bg2);
        void set_border_color(Color border);
        void set_corner_radius(CornerRadius radius);
        void set_bottom_alpha(float alpha);

        void draw(GraphicsContext &gc) override;

    protected:
        Color m_bg1;
        Color m_bg2;
        Color m_border_color;
        CornerRadius m_corner_radius;
        float m_bottom_alpha{1.0f};
    };
} // namespace horizon
