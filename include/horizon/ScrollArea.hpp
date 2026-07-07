#pragma once

#include <horizon/AquaPolygon.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    class WaylandWindow;
    /**
     * @class ScrollArea
     * @brief A widget that provides horizontal and vertical scrolling for a single child.
     */
    class ScrollArea : public Widget
    {
    public:
        ScrollArea();
        virtual ~ScrollArea();

        void set_content(std::unique_ptr<Widget> child);
        void set_scroll_position(int x, int y);

        void set_scroll_enabled(bool enabled);
        bool scroll_enabled() const { return m_scroll_enabled; }

        int scroll_x() const
        {
            return m_scroll_x;
        }
        int scroll_y() const
        {
            return m_scroll_y;
        }

        void calculate_layout() override;
        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void set_application_recursive(WaylandWindow *app) override;

        // --- Events ---
        EventsManager<EventContext> when_scroll;

    protected:
        void draw(GraphicsContext &gc) override;
        Widget *hit_test(int x, int y) override;

    private:
        void update_scrollbars();
        void handle_mouse_press(MouseButtonEventContext &ev);
        void handle_mouse_drag(MouseMoveEventContext &ev);
        void handle_mouse_release(MouseButtonEventContext &ev);
        void handle_mouse_move(MouseMoveEventContext &ev);

        int m_scroll_x{0};
        int m_scroll_y{0};

        bool m_show_h_scroll{false};
        bool m_show_v_scroll{false};

        int m_h_track_x, m_h_track_y, m_h_track_w, m_h_track_h;
        int m_v_track_x, m_v_track_y, m_v_track_w, m_v_track_h;

        std::unique_ptr<AquaPolygon> m_h_thumb;
        std::unique_ptr<AquaPolygon> m_v_thumb;

        bool m_dragging_v{false};
        bool m_dragging_h{false};
        int m_drag_start_pos{0};
        int m_drag_start_scroll{0};

        bool m_scroll_enabled{true};

        static constexpr int SCROLLBAR_SIZE = 12;
    };
} // namespace horizon
