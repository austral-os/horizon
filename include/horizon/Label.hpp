#pragma once

#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon
{
    /**
     * @brief Text alignment options for the Label widget.
     */
    enum class TextAlignment
    {
        Left,
        Center,
        Right
    };

    /**
     * @brief A widget that displays multi-line, styled text.
     */
    class Label : public Widget
    {
    public:
        Label();
        explicit Label(const std::string &text);
        ~Label() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_alignment(TextAlignment alignment);
        TextAlignment alignment() const;

        void set_font_weight(FontWeight weight);
        FontWeight font_weight() const;

        void set_font_slant(FontSlant slant);
        FontSlant font_slant() const;

        void set_font_size(int size);
        int font_size() const
        {
            return m_font_size;
        }

        void set_text_color(Color color);
        Color text_color() const
        {
            return m_text_color;
        }

    private:
        std::vector<std::string> calculate_lines(GraphicsContext &gc, int max_width);

        std::string m_text;
        TextAlignment m_alignment{TextAlignment::Left};
        FontWeight m_font_weight{FONT_WEIGHT_NORMAL};
        FontSlant m_font_slant{FONT_SLANT_NORMAL};
        int m_font_size{-1};                         // -1 means use theme default
        Color m_text_color{0.0f, 0.0f, 0.0f, -1.0f}; // a < 0 means use theme default
    };

} // namespace horizon
