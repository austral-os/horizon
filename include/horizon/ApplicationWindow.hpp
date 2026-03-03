#pragma once

#include <horizon/Toolbar.hpp>
#include <horizon/Window.hpp>
#include <string>

namespace horizon
{
    class ApplicationWindow : public Window
    {
    public:
        ApplicationWindow(std::string title);
        ~ApplicationWindow() = default;

        Toolbar *toolbar() const;
    };
} // namespace horizon
