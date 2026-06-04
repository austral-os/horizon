#pragma once
#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <utils/XkbParser.hpp>
#include <vector>
#include <memory>
#include <horizon/ConfigManager.hpp>

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
        void apply_layout_to_labwc(const std::string& layout_id);
        void apply_layout_to_meteor(const std::string& layout_id);

        TableView<KeyboardLayoutSelection>* m_layout_table{nullptr};
        std::vector<KeyboardLayoutSelection> m_selected_layouts;
        std::unique_ptr<ConfigManager> m_config;
    };
} // namespace horizon::preferences
