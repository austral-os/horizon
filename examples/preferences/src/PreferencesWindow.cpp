#include "PreferencesWindow.hpp"

namespace horizon::preferences
{
    PreferencesWindow::PreferencesWindow() : ApplicationWindow("Preferencias del Sistema")
    {
        set_size(800, 600);

        // Custom Toolbar
        auto toolbar_widget = std::make_unique<PreferencesToolbar>();
        m_preferences_toolbar = toolbar_widget.get();
        this->toolbar()->add_toolbar_widget(std::move(toolbar_widget));

        // Content View
        auto content = std::make_unique<ContentView>();
        m_content_view = content.get();
        
        // Initial Panel
        auto home_panel = std::make_unique<ViewPanel>();
        m_content_view->load_view(std::move(home_panel));

        set_content(std::move(content));
    }
} // namespace horizon::preferences
