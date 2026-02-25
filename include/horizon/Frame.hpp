#pragma once

#include <horizon/Widget.hpp>
namespace horizon
{

    class Frame : public Widget
    {
    public:
        Frame();
        ~Frame() = default;

        void draw(GraphicsContext &gc) override;
    };

} // namespace horizon