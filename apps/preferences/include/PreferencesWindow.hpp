#pragma once

#include <horizon/ConfigManager.hpp>
#include <horizon/ApplicationWindow.hpp>
#include "PreferencesToolbar.hpp"
#include "ContentView.hpp"
#include <vector>
#include <string>
#include <memory>

namespace horizon::preferences
{
    class PreferencesWindow : public ApplicationWindow
    {
    public:
        PreferencesWindow(const std::string& initial_section = "home");
        ~PreferencesWindow() override = default;

        void load_view_by_id(const std::string& id, bool push_to_history = true);
        void go_back();
        void go_forward();

    private:
        void update_navigation_buttons();
        void connect_view_signals(Widget* view);

    private:
        PreferencesToolbar* m_preferences_toolbar{nullptr};
        ContentView* m_content_view{nullptr};

        std::vector<std::string> m_history;
        int m_history_index{-1};
        std::string m_last_selected_network_path;
    };

} // namespace horizon::preferences
