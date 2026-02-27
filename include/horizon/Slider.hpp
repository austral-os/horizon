#pragma once

#include <functional>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    enum class SliderOrientation
    {
        Horizontal,
        Vertical
    };

    /**
     * @brief An Aqua Tiger-style slider widget.
     *
     * Renders a recessed track with a diamond-shaped blue thumb.
     * Supports horizontal and vertical orientations.
     * Optional tick marks can be shown below/right of the track.
     */
    class Slider : public Widget
    {
    public:
        Slider();

        void draw(GraphicsContext &gc) override;

        // --- Value ---
        void set_value(float value); // 0.0 – 1.0
        float value() const;

        // --- Range ---
        void set_min(float min);
        void set_max(float max);

        // --- Orientation ---
        void set_orientation(SliderOrientation orientation);
        SliderOrientation orientation() const;

        // --- Ticks ---
        void set_tick_count(int count); // 0 = no ticks

        // --- Callback ---
        void set_on_value_changed(std::function<void(float)> cb);

    private:
        // Helpers
        void handle_mouse_press(EventContext &ev);
        void handle_mouse_drag(EventContext &ev);
        void update_value_from_pos(int x, int y);
        int thumb_center() const; // pixel offset of thumb along track axis

        float m_value{0.5f};
        float m_min{0.0f};
        float m_max{1.0f};
        SliderOrientation m_orientation{SliderOrientation::Horizontal};
        int m_tick_count{0};
        bool m_dragging{false};

        std::function<void(float)> m_on_value_changed;
    };
} // namespace horizon
