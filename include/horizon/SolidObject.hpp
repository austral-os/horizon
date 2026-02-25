#pragma once
#include <horizon/Widget.hpp>

namespace horizon
{

    class SolidObject : public Widget
    {
    public:
        SolidObject();
        ~SolidObject() = default;

        void draw(GraphicsContext &gc) override;
    };

} // namespace horizon