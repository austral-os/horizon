#pragma once

#include "horizon/CompositorContext.hpp"
#include "horizon/Window.hpp"

namespace horizon
{
    class Window;

    /**
     * @class WayfireCompositorContext
     * @brief Implementation of CompositorContext for Wayfire/XDG-Shell based environments.
     */
    class WayfireCompositorContext : public CompositorContext
    {
    public:
        explicit WayfireCompositorContext(Window *window);
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
        Window *m_window;
    };
} // namespace horizon
