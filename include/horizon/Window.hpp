#pragma once

#include "Widget.hpp"
#include <string>

namespace horizon
{

    class Window : public Widget
    {
    public:
        explicit Window(std::string title);

        void setSize(int width, int height);

        const std::string &title() const;

        void render(GraphicsContext &gc) override;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        std::string m_title;
    };

} // namespace horizon