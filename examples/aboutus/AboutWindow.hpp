#pragma once

#include <horizon/ApplicationWindow.hpp>

namespace horizon
{
    class AboutWindow : public ApplicationWindow
    {
    public:
        AboutWindow();
        ~AboutWindow() = default;

    private:
        void set_content(std::unique_ptr<Widget> content);
    };
} // namespace horizon