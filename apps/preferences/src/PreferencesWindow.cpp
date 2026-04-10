#include "PreferencesWindow.hpp"
#include "ConfigManager.hpp"
#include "ViewPanel.hpp"
#include <horizon/I18n.hpp>
#include "views/AppearanceView/AppearanceView.hpp"
#include "views/ApplicationsView/ApplicationsView.hpp"
#include "views/BluetoothView/BluetoothView.hpp"
#include "views/DateTimeView/DateTimeView.hpp"
#include "views/DesktopView/DesktopView.hpp"
#include "views/DetailsView/DetailsView.hpp"
#include "views/DisplayView/DisplayView.hpp"
#include "views/KeyboardView/KeyboardView.hpp"
#include "views/MouseView/MouseView.hpp"
#include "views/NetworkView/NetworkView.hpp"
#include "views/NotificationsView/NotificationsView.hpp"
#include "views/PowerView/PowerView.hpp"
#include "views/PrintersView/PrintersView.hpp"
#include "views/RegionView/RegionView.hpp"
#include "views/ScreensaverView/ScreensaverView.hpp"
#include "views/SoundView/SoundView.hpp"
#include "views/UsersView/UsersView.hpp"
#include "views/WifiView/WifiView.hpp"

namespace horizon::preferences
{
    PreferencesWindow::PreferencesWindow(const std::string &initial_section)
        : ApplicationWindow(i18n().tr("preferences.title"))
    {
        ConfigManager::instance().load();
        set_size(800, 650);

        // Custom Toolbar
        auto toolbar_widget = std::make_unique<PreferencesToolbar>();
        m_preferences_toolbar = toolbar_widget.get();
        this->toolbar()->add_toolbar_widget(std::move(toolbar_widget));

        // Connect Navigation Buttons
        if (m_preferences_toolbar->navigation())
        {
            m_preferences_toolbar->navigation()->when_button_clicked.connect(
                [this](GroupButtonClickEvent &ev)
                {
                    if (ev.button_index == 0)
                        go_back();
                    else if (ev.button_index == 1)
                        go_forward();
                });
        }

        // Connect Home Button
        if (m_preferences_toolbar->home_button())
        {
            m_preferences_toolbar->home_button()->when_button_clicked.connect(
                [this](GroupButtonClickEvent &) { load_view_by_id("home"); });
        }

        // Content View
        auto content = std::make_unique<ContentView>();
        m_content_view = content.get();
        set_content(std::move(content));

        // Initial Panel
        load_view_by_id(initial_section);
    }

    void PreferencesWindow::load_view_by_id(const std::string &id, bool push_to_history)
    {
        std::unique_ptr<Widget> view;
        if (id == "home")
            view = std::make_unique<ViewPanel>();
        else if (id == "desktop")
            view = std::make_unique<DesktopView>();
        else if (id == "appearance")
            view = std::make_unique<AppearanceView>();
        else if (id == "screensaver")
            view = std::make_unique<ScreensaverView>();
        else if (id == "notifications")
            view = std::make_unique<NotificationsView>();
        else if (id == "display")
            view = std::make_unique<DisplayView>();
        else if (id == "sound")
            view = std::make_unique<SoundView>();
        else if (id == "mouse")
            view = std::make_unique<MouseView>();
        else if (id == "keyboard")
            view = std::make_unique<KeyboardView>();
        else if (id == "printers")
            view = std::make_unique<PrintersView>();
        else if (id == "power")
            view = std::make_unique<PowerView>();
        else if (id == "wi-fi")
            view = std::make_unique<WifiView>();
        else if (id == "bluetooth")
            view = std::make_unique<BluetoothView>();
        else if (id == "network")
            view = std::make_unique<NetworkView>();
        else if (id == "users")
            view = std::make_unique<UsersView>();
        else if (id == "datetime")
            view = std::make_unique<DateTimeView>();
        else if (id == "region")
            view = std::make_unique<RegionView>();
        else if (id == "details")
            view = std::make_unique<DetailsView>();
        else if (id == "applications")
            view = std::make_unique<ApplicationsView>();

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

    void PreferencesWindow::connect_view_signals(Widget *view)
    {
        if (auto home_panel = dynamic_cast<ViewPanel *>(view))
        {
            home_panel->when_item_click.connect([this](const GroupedIconItem &item)
                                                { load_view_by_id(item.id); });
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
        if (!m_preferences_toolbar)
            return;

        if (m_preferences_toolbar->navigation())
        {
            auto nav = m_preferences_toolbar->navigation();
            if (nav->children().size() >= 2)
            {
                nav->children()[0]->set_enabled(m_history_index > 0);
                nav->children()[1]->set_enabled(m_history_index < (int)m_history.size() - 1);
            }
        }

        if (m_preferences_toolbar->home_button())
        {
            bool is_home = (m_history_index >= 0 && m_history[m_history_index] == "home");
            m_preferences_toolbar->home_button()->set_visible(!is_home);
        }
    }
} // namespace horizon::preferences
