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
     * @brief Vertical alignment options for the Label widget.
     */
    enum class VerticalAlignment
    {
        Top,
        Middle,
        Bottom
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

        int preferred_width() const override;
        int preferred_height() const override;
        int preferred_height(int width) const override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_alignment(TextAlignment alignment);
        TextAlignment alignment() const;

        void set_vertical_alignment(VerticalAlignment alignment);
        VerticalAlignment vertical_alignment() const;

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

        void set_left_padding(int padding)
        {
            m_left_padding = padding;
            invalidate();
        }
        int left_padding() const
        {
            return m_left_padding;
        }

    private:
        std::vector<std::string> calculate_lines(GraphicsContext &gc, int max_width, int max_height,
                                                 int line_height);

        std::string m_text;
        TextAlignment m_alignment{TextAlignment::Left};
        VerticalAlignment m_vertical_alignment{VerticalAlignment::Middle};
        FontWeight m_font_weight{FONT_WEIGHT_NORMAL};
        FontSlant m_font_slant{FONT_SLANT_NORMAL};
        int m_font_size{-1};                         // -1 means use theme default
        Color m_text_color{0.0f, 0.0f, 0.0f, -1.0f}; // a < 0 means use theme default
        int m_left_padding{0};

        // Cache for layout results
        std::vector<std::string> m_cached_lines;
        int m_last_width{-1};
        int m_last_height{-1};
        std::string m_last_text;
        FontWeight m_last_font_weight{FONT_WEIGHT_NORMAL};
        int m_last_font_size{-1};
    };

} // namespace horizon
