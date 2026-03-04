#pragma once
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    class Dialog
    {
    public:
        virtual ~Dialog() = default;

        virtual const std::string &id() const = 0;
        virtual Widget *root_widget() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
    };
} // namespace horizon
