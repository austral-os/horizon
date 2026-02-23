#pragma once

#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"

namespace horizon
{

    class TitlebarCircleButton : public Widget
    {
    public:
        TitlebarCircleButton(Color color);
        ~TitlebarCircleButton();

        void draw(GraphicsContext &gc) override;

    private:
        Color m_bg_color;
    };
} // namespace horizon