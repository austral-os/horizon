#pragma once

#include "horizon/GraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <functional>

namespace horizon
{

    class TitlebarCircleButton : public Widget
    {
    public:
        TitlebarCircleButton(Color color);
        ~TitlebarCircleButton();

        void draw(GraphicsContext &gc) override;
        void on_mouse_press(int button) override;

        void set_on_click(std::function<void()> on_click);

    private:
        Color m_bg_color;
        std::function<void()> m_on_click;
    };
} // namespace horizon