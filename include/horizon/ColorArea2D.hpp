#pragma once

#include <horizon/Color.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @class ColorArea2D
     * @brief A 2D selection field for choosing color components (e.g., Saturation and Value).
     */
    class ColorArea2D : public Widget
    {
    public:
        ColorArea2D();
        virtual ~ColorArea2D();

        void draw(GraphicsContext &gc) override;

        // --- Configuration ---
        void set_hue(float hue); // 0.0 to 1.0 (used if mode is HSV)

        void set_values(float x_val, float y_val);
        float value_x() const
        {
            return m_val_x;
        }
        float value_y() const
        {
            return m_val_y;
        }

        // --- Events ---
        EventsManager<EventContext>
            when_values_changed; // ev.data = this (caller should read value_x/y)

    private:
        void handle_mouse_press(MouseButtonEventContext &ev);
        void handle_mouse_drag(MouseMoveEventContext &ev);
        void update_values_from_pos(int x, int y);

        float m_hue{0.0f};
        float m_val_x{0.0f};
        float m_val_y{0.0f};
        bool m_dragging{false};
    };
} // namespace horizon
