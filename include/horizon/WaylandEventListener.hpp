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

        Type type{Type::Move};
        double x{0.0};
        double y{0.0};
        uint32_t button{0};
    };

    struct KeyEvent
    {
        enum class Type
        {
            Press,
            Release
        };

        Type type{Type::Press};
        uint32_t key{0};
    };

    class WaylandEventListener
    {
    public:
        virtual ~WaylandEventListener() = default;

        virtual void on_pointer_event(const PointerEvent &event) = 0;
        virtual void on_key_event(const KeyEvent &event) = 0;
    };
} // namespace horizon