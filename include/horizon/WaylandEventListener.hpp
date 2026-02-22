#pragma once

#include <cstdint>
namespace horizon
{

    struct PointerEvent
    {
        enum class Type
        {
            Move,
            Press,
            Release,
            Enter,
            Leave,
            Scroll
        };

        Type type;
        double x;
        double y;
        uint32_t button = 0;
    };

    class WaylandEventListener
    {
    public:
        virtual ~WaylandEventListener() = default;

        virtual void on_pointer_event(const PointerEvent &event) = 0;
    };
} // namespace horizon