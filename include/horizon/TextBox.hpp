#pragma once
#include <chrono>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @class TextBoxBase
     * @brief Non-templated base for TextBox to avoid code bloat and maintain compatibility.
     */
    class TextBoxBase : public Widget
    {
    public:
        TextBoxBase();
        virtual ~TextBoxBase() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_placeholder(const std::string &placeholder);
        const std::string &placeholder() const;

        virtual bool is_valid() const = 0;
        virtual std::string get_display_text() const = 0;

        EventsManager when_text_changed;

    protected:
        std::string m_text{""};
        std::string m_placeholder{""};
        int m_cursor_pos{0};
        int m_selection_anchor{-1};
        int m_scroll_offset{0};
        bool m_cursor_visible{true};
        bool m_is_dragging{false};
        std::chrono::steady_clock::time_point m_last_blink_time;
        int m_pending_click_x{-1};
        bool m_has_pending_click{false};
    };

    template <typename Policy = TextPolicy> class TextBox : public TextBoxBase
    {
    public:
        TextBox() = default;
        ~TextBox() = default;

        bool is_valid() const override
        {
            return Policy::validate(m_text, config);
        }

        std::string get_display_text() const override
        {
            return Policy::get_display_text(m_text, config);
        }

        TextBoxConfig config;
    };
} // namespace horizon
