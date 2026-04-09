#include "horizon/Statusbar.hpp"
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Toolbar.hpp>

namespace horizon
{
    void ApplicationWindow::set_immersive_mode(bool immersive)
    {
        m_is_immersive = immersive;
        if (m_titlebar)
            m_titlebar->set_visible(!immersive);
        
        if (m_content)
            m_content->set_margin(immersive ? 0 : 1);
        
        invalidate();
    }

    ApplicationWindow::ApplicationWindow(std::string title)
        : Window(std::make_unique<Toolbar>(std::move(title)))
    {
        auto content = std::make_unique<Widget>();
        content->set_margin(1);
        m_content = content.get();
        add_child(std::move(content));
    }

    Toolbar *ApplicationWindow::toolbar() const
    {
        // m_titlebar is Titlebar*, but in ApplicationWindow it points to a Toolbar
        return static_cast<Toolbar *>(m_titlebar);
    }

    Statusbar *ApplicationWindow::statusbar() const
    {
        return m_status_bar;
    }

    void ApplicationWindow::set_content(std::unique_ptr<Widget> content)
    {
        m_content->clear_children();
        m_content->add_child(std::move(content));
    }

    Widget *ApplicationWindow::content() const
    {
        if (m_content->children().empty())
            return nullptr;
        return m_content->children()[0].get();
    }

    void ApplicationWindow::show_status_bar()
    {
        if (m_status_bar)
            return;
        auto sb = std::make_unique<Statusbar>();
        sb->set_fixed_size(30);
        m_status_bar = sb.get();
        add_child(std::move(sb));
    }

    void ApplicationWindow::hide_status_bar()
    {
        if (m_status_bar)
        {
            remove_child(m_status_bar);
            m_status_bar = nullptr;
        }
    }

    void ApplicationWindow::set_status_text(std::string text)
    {
        if (m_status_bar)
        {
            m_status_bar->set_text(std::move(text));
        }
    }

    CornerRadius ApplicationWindow::get_window_corners() const
    {
        if (m_is_immersive)
        {
            return {0, 0, 0, 0};
        }
        if (m_status_bar)
        {
            return {10, 10, 10, 10};
        }
        return {10, 10, 0, 0};
    }

} // namespace horizon
