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

        void draw(GraphicsContext &gc) override;

    protected:
        CornerRadius m_corner_radius;
    };

} // namespace horizon