#pragma once
#include <horizon/GraphicsContext.hpp>
#include <horizon/BorderConfig.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @brief AirObject is a lightweight, rounded container with a soft gradient effect.
     * 
     * It mimics the visual style of a glassmorphic or "air" themed UI element.
     */
    class AirObject : public Widget
    {
    public:
        AirObject();
        virtual ~AirObject() = default;

        void set_corner_radius(CornerRadius radius);
        CornerRadius corner_radius() const;

        void draw(GraphicsContext &gc) override;

    protected:
        CornerRadius m_corner_radius;
    };
} // namespace horizon
