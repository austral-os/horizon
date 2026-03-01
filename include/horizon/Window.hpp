#pragma once

#include "Widget.hpp"
#include "horizon/Titlebar.hpp"
#include <string>

namespace horizon
{

    class Window : public Widget
    {
    public:
        explicit Window(std::string title);

        void set_size(int width, int height);

        const std::string &title() const;

        void render(GraphicsContext &gc, bool force = false) override;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        Titlebar *m_titlebar;
    };

} // namespace horizon