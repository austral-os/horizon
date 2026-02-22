#pragma once

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

    private:
        std::string m_title;
    };
} // namespace horizon