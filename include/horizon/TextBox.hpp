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
        bool supports_clipboard() const override { return true; }

        void set_text(const std::string &text);
        const std::string &text() const;

        void select_all();
        
        // Clipboard Support
        bool can_perform(ClipboardAction action) const override;
        void perform(ClipboardAction action) override;
        void provide_clipboard_data(const std::string &mime, DataSink &sink) override;
        std::vector<std::string> provided_mime_types() const override;
        std::vector<std::string> accepted_mime_types() const override;
        void on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data) override;

        void set_placeholder(const std::string &placeholder);
        const std::string &placeholder() const;

        void set_font_family(const std::string &family);
        const std::string &font_family() const;

        virtual bool is_valid() const = 0;
        virtual std::string get_display_text() const = 0;

        EventsManager<KeyEventContext> when_text_changed;

    protected:
        std::string m_text{""};
        std::string m_placeholder{""};
        std::string m_font_family{""};
        int m_cursor_pos{0};
        int m_selection_anchor{-1};
        int m_scroll_offset{0};
        bool m_cursor_visible{true};
        bool m_is_dragging{false};
        std::chrono::steady_clock::time_point m_last_blink_time;
        int m_pending_click_x{-1};
        bool m_has_pending_click{false};

        // UI Customization
        int m_padding_left{8};
        int m_padding_right{8};
        int m_corner_radius{0};
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
