#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/TableView.hpp>
#include <horizon/DesktopManager.hpp>
#include <string>
#include <vector>

namespace horizon
{
    class AppPickerDialog : public WaylandWindow
    {
    public:
        AppPickerDialog();
        ~AppPickerDialog() override = default;

        EventsManager<DesktopEntry> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui();
        void load_apps();
        void filter_apps(const std::string& query);

        SearchBox* m_search_box{nullptr};
        TableView<DesktopEntry>* m_app_table{nullptr};
        
        std::vector<DesktopEntry> m_all_apps;
        std::vector<DesktopEntry> m_filtered_apps;
    };
} // namespace horizon
