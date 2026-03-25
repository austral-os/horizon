#pragma once

#include <horizon/Widget.hpp>

namespace horizon
{

    /**
     * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
     */
    class DockShelf : public Widget
    {
    public:
        DockShelf();

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

        void set_magnification_enabled(bool enabled) { m_magnification_enabled = enabled; }
        bool is_magnification_enabled() const { return m_magnification_enabled; }
        void set_base_size(int size) { m_base_size = size; m_max_extra_size = size; calculate_layout(); invalidate(); }
        void reset_magnification() { m_mouse_over = false; calculate_layout(); invalidate(); }
        int base_size() const { return m_base_size; }

    private:
        bool m_magnification_enabled = true;
        int m_base_size = 64;
        int m_max_extra_size = 64;
        int m_mouse_x = -1;
        int m_mouse_y = -1;
        bool m_mouse_over = false;
    };

} // namespace horizon
