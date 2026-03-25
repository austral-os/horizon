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
        void reset_magnification() { m_mouse_over = false; calculate_layout(); invalidate(); }

    private:
        bool m_magnification_enabled = true;
        int m_mouse_x = -1;
        int m_mouse_y = -1;
        bool m_mouse_over = false;
    };

} // namespace horizon
