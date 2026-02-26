#pragma once

#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    class TextBox : public Widget
    {
    public:
        TextBox();
        ~TextBox() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_placeholder(const std::string &placeholder);
        const std::string &placeholder() const;

    protected:
        std::string m_text{""};
        std::string m_placeholder{""};
        int m_cursor_pos{0};
    };
} // namespace horizon
