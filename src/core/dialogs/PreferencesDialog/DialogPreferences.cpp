#include "DialogPreferences.hpp"
#include "PreferencesToolbar.hpp"
#include <horizon/Toolbar.hpp>

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

    void DialogPreferences::setup_toolbar(PreferencesContent *content)
    {
        if (m_app_window && content)
        {
            auto toolbar_widget = std::make_unique<PreferencesToolbar>(content);
            m_app_window->toolbar()->add_toolbar_widget(std::move(toolbar_widget));
        }
    }
} // namespace horizon
