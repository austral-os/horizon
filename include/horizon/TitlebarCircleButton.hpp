#pragma once

#include "horizon/Widget.hpp"
namespace horizon
{

    class TitlebarCircleButton : public Widget
    {
    public:
        TitlebarCircleButton();
        ~TitlebarCircleButton();

        void draw(GraphicsContext &gc) override;
    };
} // namespace horizon