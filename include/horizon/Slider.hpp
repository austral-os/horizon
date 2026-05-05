#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    enum class SliderOrientation
    {
        Horizontal,
        Vertical
    };

    enum class ThumbShape
    {
        Marker,
        Circle
    };

    /**
     * @brief An Aqua Tiger-style slider widget.
     *
     * Renders a recessed track with a diamond-shaped or circular blue thumb.
     * Supports horizontal and vertical orientations.
     * Optional tick marks can be shown below/right of the track.
     *
     * Value changes are notified via when_value_changed. The current float
     * value is passed as ev.data (reinterpret_cast<float*>(ev.data)).
     */
    class WaylandWindow;
    class AquaPolygon;
    class Slider : public Widget
    {
    public:
        Slider();
        virtual ~Slider();

        void set_application_recursive(WaylandWindow *app) override;

        void draw(GraphicsContext &gc) override;

        // --- Value ---
        void set_value(float value); // 0.0 - 1.0 (or min value in range mode)
        float value() const;

        void set_second_value(float value); // Only used if range enabled
        float second_value() const;

        void set_enable_range(bool enable);
        bool range_enabled() const;

        // --- Range ---
        void set_min(float min);
        void set_max(float max);

        // --- Orientation ---
        void set_orientation(SliderOrientation orientation);
        SliderOrientation orientation() const;

        // --- Ticks ---
        void set_tick_count(int count); // 0 = no ticks
        void set_show_ticks(bool show);
        bool show_ticks() const;

        void add_custom_tick(float value);
        void clear_custom_ticks();

        // --- Thumb ---
        void set_thumb_shape(ThumbShape shape);
        ThumbShape thumb_shape() const;

        // --- Events ---
        EventsManager<EventContext>
            when_value_changed; // ev.data = reinterpret_cast<float*>(&value)
        EventsManager<EventContext>
            when_changed; // Fires when the user finishes dragging

    private:
        // Helpers
        void handle_mouse_press(MouseButtonEventContext &ev);
        void handle_mouse_drag(MouseMoveEventContext &ev);
        void update_value_from_pos(int x, int y);
        int thumb_center(float val) const; // pixel offset of thumb along track axis
        void update_thumb_polygons();

        float m_value{0.5f};
        float m_second_value{0.75f};
        float m_min{0.0f};
        float m_max{1.0f};
        bool m_enable_range{false};

        SliderOrientation m_orientation{SliderOrientation::Horizontal};
        int m_tick_count{0};
        std::vector<float> m_custom_ticks;
        bool m_show_ticks{true};
        ThumbShape m_thumb_shape{ThumbShape::Marker};
        bool m_dragging_first{false};
        bool m_dragging_second{false};

        std::unique_ptr<AquaPolygon> m_thumb_poly;
        std::unique_ptr<AquaPolygon> m_second_thumb_poly;
    };
} // namespace horizon
