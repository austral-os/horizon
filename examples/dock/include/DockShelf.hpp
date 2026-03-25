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

    private:
        bool m_magnification_enabled = true;
        int m_mouse_x = -1;
        bool m_mouse_over = false;
    };

} // namespace horizon
