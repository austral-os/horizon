#pragma once

#include "Widget.hpp"
#include "horizon/Titlebar.hpp"
#include <horizon/GraphicsContext.hpp>
#include <string>

namespace horizon
{

    class Window : public Widget
    {
    public:
        explicit Window(std::string title);

        void set_size(int width, int height);

        const std::string &title() const;
        void set_title(std::string title);

        Titlebar *titlebar() const { return m_titlebar; }

        virtual CornerRadius get_window_corners() const;

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;

    protected:
        explicit Window(std::unique_ptr<Titlebar> custom_titlebar);

        void draw(GraphicsContext &gc) override;
        Titlebar *m_titlebar;
    };

} // namespace horizon