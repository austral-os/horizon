#pragma once

#include "horizon/CompositorContext.hpp"
#include "horizon/Window.hpp"

namespace horizon
{
    class Window;

    /**
     * @class LabwcCompositorContext
     * @brief Implementation of CompositorContext for Labwc/XDG-Shell based environments.
     */
    class LabwcCompositorContext : public CompositorContext
    {
    public:
        explicit LabwcCompositorContext(Window *window);
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
        Window *m_window;
    };
} // namespace horizon
