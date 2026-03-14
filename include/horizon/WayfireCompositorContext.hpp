#pragma once

#include "horizon/CompositorContext.hpp"

namespace horizon
{
    class WaylandWindow;

    /**
     * @class WayfireCompositorContext
     * @brief Implementation of CompositorContext for Wayfire/XDG-Shell based environments.
     */
    class WayfireCompositorContext : public CompositorContext
    {
    public:
        explicit WayfireCompositorContext(WaylandWindow *app);
        ~WayfireCompositorContext() override = default;

        void request_move(uint32_t serial) override;
        void request_resize(uint32_t serial, uint32_t edge) override;
        void maximize() override;
        void minimize() override;
        void restore(const std::string &token = "") override;
        void fullscreen() override;
        void unfullscreen() override;
        void set_blur(bool enabled) override;

        bool is_maximized() const override;
        bool is_fullscreen() const override;

    private:
        WaylandWindow *m_app;
    };
} // namespace horizon
