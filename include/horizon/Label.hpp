#pragma once

#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>
#include <horizon/TextBox.hpp>

namespace horizon
{
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

        void set_font_family(const std::string &family);
        const std::string &font_family() const
        {
            return m_font_family;
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

        void set_has_shadow(bool shadow)
        {
            if (m_has_shadow != shadow) {
                m_has_shadow = shadow;
                invalidate();
            }
        }
        bool has_shadow() const
        {
            return m_has_shadow;
        }

        void set_editable(bool editable);
        bool is_editable() const;

        void begin_edit();
        void end_edit(bool accept_changes = true);

        EventsManager<EventContext> when_text_edited;

    private:
        std::vector<std::string> calculate_lines(GraphicsContext &gc, int max_width, int max_height,
                                                 int line_height);

        std::string m_text;
        TextAlignment m_alignment{TextAlignment::Left};
        VerticalAlignment m_vertical_alignment{VerticalAlignment::Middle};
        FontWeight m_font_weight{FONT_WEIGHT_NORMAL};
        FontSlant m_font_slant{FONT_SLANT_NORMAL};
        int m_font_size{-1};                         // -1 means use theme default
        std::string m_font_family{""};               // empty means use theme default
        Color m_text_color{0.0f, 0.0f, 0.0f, -1.0f}; // a < 0 means use theme default
        int m_left_padding{0};
        bool m_has_shadow{false};

        bool m_editable{false};
        bool m_is_editing{false};
        TextBox<TextPolicy>* m_editor{nullptr};

        // Cache for layout results
        std::vector<std::string> m_cached_lines;
        int m_last_width{-1};
        int m_last_height{-1};
        std::string m_last_text;
        FontWeight m_last_font_weight{FONT_WEIGHT_NORMAL};
        int m_last_font_size{-1};
        std::string m_last_font_family{""};
    };

} // namespace horizon
