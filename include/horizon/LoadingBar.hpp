#pragma once

#include <horizon/Widget.hpp>

namespace horizon
{
    class WaylandWindow;

    /**
     * @brief A loading bar widget with a continuous "busy" animation.
     * Inspired by classic macOS (Tiger/Leopard) indeterminate progress bars.
     */
    class LoadingBar : public Widget
    {
    public:
        LoadingBar();
        ~LoadingBar();

        void draw(GraphicsContext &gc) override;
        void set_application_recursive(WaylandWindow *app) override;

    private:
        float m_animation_offset{0.0f};
        size_t m_timer_id{0};
    };
} // namespace horizon
