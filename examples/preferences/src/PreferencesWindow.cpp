#include "PreferencesWindow.hpp"
#include "views/DesktopView.hpp"
#include "ViewPanel.hpp"

namespace horizon::preferences
{
    PreferencesWindow::PreferencesWindow() : ApplicationWindow("Preferencias del Sistema")
    {
        set_size(800, 600);

        // Custom Toolbar
        auto toolbar_widget = std::make_unique<PreferencesToolbar>();
        m_preferences_toolbar = toolbar_widget.get();
        this->toolbar()->add_toolbar_widget(std::move(toolbar_widget));

        // Connect Navigation Buttons
        if (m_preferences_toolbar->navigation())
        {
            m_preferences_toolbar->navigation()->when_button_clicked.connect([this](GroupButtonClickEvent &ev) {
                if (ev.button_index == 0) go_back();
                else if (ev.button_index == 1) go_forward();
            });
        }

        // Content View
        auto content = std::make_unique<ContentView>();
        m_content_view = content.get();
        set_content(std::move(content));

        // Initial Panel
        load_view_by_id("home");
    }

    void PreferencesWindow::load_view_by_id(const std::string& id, bool push_to_history)
    {
        std::unique_ptr<Widget> view;
        if (id == "home")
        {
            view = std::make_unique<ViewPanel>();
        }
        else if (id == "desktop")
        {
            view = std::make_unique<DesktopView>();
        }

        if (view)
        {
            connect_view_signals(view.get());
            m_content_view->load_view(std::move(view));

            if (push_to_history)
            {
                // Clear forward history
                if (m_history_index < (int)m_history.size() - 1)
                {
                    m_history.erase(m_history.begin() + m_history_index + 1, m_history.end());
                }
                m_history.push_back(id);
                m_history_index = m_history.size() - 1;
            }
            update_navigation_buttons();
        }
    }

    void PreferencesWindow::connect_view_signals(Widget* view)
    {
        if (auto home_panel = dynamic_cast<ViewPanel*>(view))
        {
            home_panel->when_item_click.connect([this](const GroupedIconItem& item) {
                load_view_by_id(item.id);
            });
        }
    }

    void PreferencesWindow::go_back()
    {
        if (m_history_index > 0)
        {
            m_history_index--;
            load_view_by_id(m_history[m_history_index], false);
        }
    }

    void PreferencesWindow::go_forward()
    {
        if (m_history_index < (int)m_history.size() - 1)
        {
            m_history_index++;
            load_view_by_id(m_history[m_history_index], false);
        }
    }

    void PreferencesWindow::update_navigation_buttons()
    {
        if (!m_preferences_toolbar || !m_preferences_toolbar->navigation()) return;

        auto nav = m_preferences_toolbar->navigation();
        if (nav->children().size() >= 2)
        {
            nav->children()[0]->set_enabled(m_history_index > 0);
            nav->children()[1]->set_enabled(m_history_index < (int)m_history.size() - 1);
        }
    }
} // namespace horizon::preferences
