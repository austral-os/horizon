#pragma once
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{

    class SolidObject : public Widget
    {
    public:
        SolidObject();
        ~SolidObject() = default;

        void draw(GraphicsContext &gc) override;
        void set_corner_radius(CornerRadius radius);
        CornerRadius corner_radius() const;

    protected:
        CornerRadius m_corner_radius;
    };

} // namespace horizon