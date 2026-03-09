#pragma once

#include <cstdint>
#include <string>

namespace horizon
{
    /**
     * @class CompositorContext
     * @brief Abstract base class for interacting with the window compositor.
     */
    class CompositorContext
    {
    public:
        virtual ~CompositorContext() = default;

        virtual void request_move(uint32_t serial) = 0;
        virtual void request_resize(uint32_t serial, uint32_t edge) = 0;
        virtual void maximize() = 0;
        virtual void minimize() = 0;
        virtual void restore(const std::string &token = "") = 0;
        virtual void fullscreen() = 0;
        virtual void unfullscreen() = 0;

        virtual bool is_maximized() const = 0;
        virtual bool is_fullscreen() const = 0;
    };
} // namespace horizon
