#pragma once

#include <horizon/ApplicationWindow.hpp>
#include "PreferencesToolbar.hpp"
#include "ContentView.hpp"
#include "ViewPanel.hpp"
#include <memory>

namespace horizon::preferences
{
    class PreferencesWindow : public ApplicationWindow
    {
    public:
        PreferencesWindow();
        ~PreferencesWindow() override = default;

    private:
        PreferencesToolbar* m_preferences_toolbar{nullptr};
        ContentView* m_content_view{nullptr};
    };
} // namespace horizon::preferences
