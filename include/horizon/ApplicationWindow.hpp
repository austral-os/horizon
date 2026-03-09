#pragma once

#include "horizon/Statusbar.hpp"
#include <horizon/Toolbar.hpp>
#include <horizon/Window.hpp>
#include <memory>
#include <string>

namespace horizon
{
    class ApplicationWindow : public Window
    {
    public:
        ApplicationWindow(std::string title);
        ~ApplicationWindow() = default;

        Toolbar *toolbar() const;
        Statusbar *statusbar() const;

        void show_status_bar();
        void hide_status_bar();

        void set_content(std::unique_ptr<Widget> content);
        Widget *content() const;

        void set_status_text(std::string text);

        CornerRadius get_window_corners() const override;

    protected:
        Widget *m_content;
        Statusbar *m_status_bar{nullptr};
    };
} // namespace horizon
