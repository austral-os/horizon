#include <horizon/DialogPreferences.hpp>

namespace horizon
{
    DialogPreferences::DialogPreferences(const std::string &title, int width, int height)
        : WaylandWindow("horizon.preferences.dialog", width, height, true, true)
    {
        set_name(title);
        
        auto app_window = std::make_unique<ApplicationWindow>(title);
        m_app_window = app_window.get();
        
        set_root(std::move(app_window));
    }

    Toolbar *DialogPreferences::toolbar() const
    {
        return m_app_window ? m_app_window->toolbar() : nullptr;
    }

    void DialogPreferences::set_content(std::unique_ptr<Widget> content)
    {
        if (m_app_window)
        {
            m_app_window->set_content(std::move(content));
        }
    }

    Widget *DialogPreferences::content() const
    {
        return m_app_window ? m_app_window->content() : nullptr;
    }
} // namespace horizon
