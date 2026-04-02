#pragma once
#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <utils/XkbParser.hpp>
#include <vector>

namespace horizon::preferences
{
    class KeyboardLanguageView : public Widget
    {
    public:
        KeyboardLanguageView();
        ~KeyboardLanguageView() override = default;

    private:
        void setup_ui();
        void load_config();
        void save_config();
        void add_layout();
        void remove_layout();
        void set_default_layout();

        TableView<KeyboardLayoutSelection>* m_layout_table{nullptr};
        std::vector<KeyboardLayoutSelection> m_selected_layouts;
    };
} // namespace horizon::preferences
