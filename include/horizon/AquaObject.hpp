#pragma once
#include "horizon/GraphicsContext.hpp"
#include <horizon/BorderConfig.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{

    class AquaObject : public Widget
    {
    public:
        AquaObject();
        ~AquaObject() = default;

        void set_corner_radius(CornerRadius radius);
        CornerRadius corner_radius() const;

        void set_color1(const Color &color) { m_c1 = color; invalidate(); }
        void set_color2(const Color &color) { m_c2 = color; invalidate(); }

        void draw(GraphicsContext &gc) override;

    protected:
        CornerRadius m_corner_radius;
        Color m_c1;
        Color m_c2;
    };

} // namespace horizon