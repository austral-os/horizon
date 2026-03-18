#pragma once

#include <horizon/ApplicationWindow.hpp>
#include "PreferencesToolbar.hpp"
#include "ContentView.hpp"
#include <vector>
#include <string>

namespace horizon::preferences
{
    class PreferencesWindow : public ApplicationWindow
    {
    public:
        PreferencesWindow();
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
    };
} // namespace horizon::preferences
