#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/TableView.hpp>
#include <utils/XkbParser.hpp>
#include <vector>
#include <string>

namespace horizon::preferences
{
    class LayoutPickerDialog : public WaylandWindow
    {
    public:
        LayoutPickerDialog();
        ~LayoutPickerDialog() override = default;

        EventsManager<KeyboardLayout> when_accepted;

    private:
        void setup_ui();
        void load_layouts();
        void filter_layouts(const std::string& query);

        SearchBox* m_search_box{nullptr};
        TableView<KeyboardLayout>* m_layout_table{nullptr};
        std::vector<KeyboardLayout> m_all_layouts;
        std::vector<KeyboardLayout> m_filtered_layouts;
    };
} // namespace horizon::preferences
