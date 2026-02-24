#pragma once
#include <horizon/Widget.hpp>

namespace horizon
{

    class AquaObject : public Widget
    {
    public:
        AquaObject();
        ~AquaObject() = default;

        void draw(GraphicsContext &gc) override;
    };

} // namespace horizon