#pragma once

#include "horizon/TitlebarCircleButton.hpp"
#include <horizon/Panel.hpp>
#include <string>

namespace horizon
{
    class Titlebar : public Panel
    {
    public:
        Titlebar(std::string title);
        ~Titlebar() = default;

        void set_title(std::string title);
        const std::string &title() const;

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void draw(GraphicsContext &gc) override;

    protected:
        std::string m_title;
        bool m_dragging_requested = false;

        TitlebarCircleButton *m_close_button;
        TitlebarCircleButton *m_minimize_button;
        TitlebarCircleButton *m_maximize_button;
    };
} // namespace horizon