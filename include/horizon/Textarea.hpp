#pragma once
#include <chrono>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon
{
    class Textarea : public Widget
    {
    public:
        Textarea();
        virtual ~Textarea() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_placeholder(const std::string &placeholder);
        const std::string &placeholder() const;

        EventsManager when_text_changed;

    protected:
        struct LineInfo
        {
            std::string text;
            int start_index;
            int length;
            int y_offset;
        };

        std::vector<LineInfo> layout_text(GraphicsContext &gc, int width_limit);
        int get_index_at(int x, int y, const std::vector<LineInfo> &lines, GraphicsContext &gc);
        void update_cursor_from_pending_click(const std::vector<LineInfo> &lines,
                                              GraphicsContext &gc);
        void ensure_cursor_visible(const std::vector<LineInfo> &lines, int visible_height);

        std::string m_text{""};
        std::string m_placeholder{""};
        int m_cursor_pos{0};
        int m_selection_anchor{-1};
        int m_scroll_offset_y{0};
        bool m_cursor_visible{true};
        bool m_is_dragging{false};
        std::chrono::steady_clock::time_point m_last_blink_time;
        int m_pending_click_x{-1};
        int m_pending_click_y{-1};
        bool m_has_pending_click{false};

        std::vector<LineInfo> m_cached_lines;

        // UI Customization
        int m_padding_left{8};
        int m_padding_right{8};
        int m_padding_top{8};
        int m_padding_bottom{8};
        int m_corner_radius{0};
        int m_line_spacing{4};
    };
} // namespace horizon
