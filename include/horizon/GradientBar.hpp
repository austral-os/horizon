#pragma once

#include <horizon/Color.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>
#include <vector>

namespace horizon
{
    /**
     * @class GradientBar
     * @brief A widget that displays a multi-stop linear gradient with a selection marker.
     *
     * Used for color component sliders and hue selection.
     */
    class GradientBar : public Widget
    {
    public:
        GradientBar();
        virtual ~GradientBar();

        void draw(GraphicsContext &gc) override;

        // --- Configuration ---
        void set_stops(const std::vector<Color> &stops);
        const std::vector<Color> &stops() const
        {
            return m_stops;
        }

        void set_value(float value); // 0.0 to 1.0
        float value() const
        {
            return m_value;
        }

        void set_vertical(bool vertical);
        bool is_vertical() const
        {
            return m_vertical;
        }

        void set_show_marker(bool show)
        {
            m_show_marker = show;
        }
        bool show_marker() const
        {
            return m_show_marker;
        }

        // --- Events ---
        EventsManager<EventContext>
            when_value_changed; // ev.data = reinterpret_cast<float*>(&m_value)

    private:
        void handle_mouse_press(MouseButtonEventContext &ev);
        void handle_mouse_drag(MouseMoveEventContext &ev);
        void update_value_from_pos(int x, int y);

        std::vector<Color> m_stops;
        float m_value{0.0f};
        bool m_vertical{false};
        bool m_show_marker{true};
        bool m_dragging{false};
    };
} // namespace horizon
