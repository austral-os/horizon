#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Combo.hpp>
#include <horizon/ConfigManager.hpp>
#include <memory>
#include <map>

namespace horizon::preferences
{
    class RegionView : public Widget
    {
    public:
        RegionView();
        ~RegionView() override = default;
    private:
        void load_languages();
        void load_formats();
        void load_timezones();

        void from_json(const nlohmann::json& j);
        nlohmann::json to_json() const;
        void save_config();
        void refresh_ui_texts();

        Label* m_title_label{nullptr};
        Combo* m_lang_combo{nullptr};
        Combo* m_formats_combo{nullptr};
        Combo* m_timezone_combo{nullptr};

        std::unique_ptr<horizon::ConfigManager> m_config;
        std::map<std::string, Label*> m_labels;
    };
}
