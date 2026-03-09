#pragma once

#include <horizon/Panel.hpp>

namespace horizon
{
    class Statusbar : public Panel
    {
    public:
        Statusbar();
        ~Statusbar() = default;

        void set_text(std::string text);
        const std::string &text() const;

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void draw(GraphicsContext &gc) override;

    private:
        std::string m_text;
    };
} // namespace horizon