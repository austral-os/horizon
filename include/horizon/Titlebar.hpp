#pragma once

#include "horizon/TitlebarCircleButton.hpp"
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    class Titlebar : public Widget
    {
    public:
        Titlebar(std::string title);
        ~Titlebar() = default;

        void set_title(std::string title);
        const std::string &title() const;

        void render(GraphicsContext &gc) override;
        void draw(GraphicsContext &gc) override;

        void on_mouse_press(int button) override;
        void on_mouse_drag(int x, int y) override;
        void on_mouse_release(int button) override;

    private:
        std::string m_title;
        bool m_dragging_requested = false;

        TitlebarCircleButton *m_close_button;
        TitlebarCircleButton *m_minimize_button;
        TitlebarCircleButton *m_maximize_button;
    };
} // namespace horizon