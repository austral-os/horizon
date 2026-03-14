#pragma once

#include "horizon/CompositorContext.hpp"

namespace horizon
{
    class WaylandWindow;

    /**
     * @class LabwcCompositorContext
     * @brief Implementation of CompositorContext for Labwc/XDG-Shell based environments.
     */
    class LabwcCompositorContext : public CompositorContext
    {
    public:
        explicit LabwcCompositorContext(WaylandWindow *app);
        ~LabwcCompositorContext() override = default;

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
