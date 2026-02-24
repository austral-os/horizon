#pragma once
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{

    class Button : public Widget
    {
    public:
        Button();
        ~Button() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(std::string text);
        const std::string &text() const;

    private:
        std::string m_text;
    };

} // namespace horizon