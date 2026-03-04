#pragma once
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief Base interface for IPC messages that produce UI elements.
     *        Renamed from Dialog for generality.
     */
    class Message
    {
    public:
        virtual ~Message() = default;

        virtual const std::string &id() const = 0;
        virtual Widget *root_widget() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
    };
} // namespace horizon
