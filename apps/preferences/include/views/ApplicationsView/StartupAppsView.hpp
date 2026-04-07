#pragma once

#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <utils/DesktopManager.hpp>
#include <vector>

namespace horizon::preferences
{
    class StartupAppsView : public Widget
    {
    public:
        StartupAppsView();
        ~StartupAppsView() override = default;

    private:
        void setup_ui();
        void load_data();
        void add_app();
        void edit_app();
        void remove_app();

        TableView<DesktopEntry>* m_table{nullptr};
        std::vector<DesktopEntry> m_data;
    };
} // namespace horizon::preferences
