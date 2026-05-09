#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/TableView.hpp>
#include <utils/TimeZoneUtils.hpp>
#include <vector>
#include <string>

namespace horizon::preferences
{
    class TimeZonePickerDialog : public WaylandWindow
    {
    public:
        TimeZonePickerDialog();
        ~TimeZonePickerDialog() override = default;

        EventsManager<TimeZone> when_accepted;

    private:
        void setup_ui();
        void load_timezones();
        void filter_timezones(const std::string& query);

        SearchBox* m_search_box{nullptr};
        TableView<TimeZone>* m_timezone_table{nullptr};
        std::vector<TimeZone> m_all_timezones;
        std::vector<TimeZone> m_filtered_timezones;
    };
} // namespace horizon::preferences
