#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/ConfigManager.hpp>
#include <memory>
#include <string>

namespace horizon::preferences
{
    class AppearanceView : public Widget, public ConfigSection
    {
    public:
        AppearanceView();
        ~AppearanceView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json &j) override;
        nlohmann::json to_json() const override;

    private:
        void setup_ui();
        void save_config();
        void update_selection_visuals();

        Label* m_title_label{nullptr};
        Widget* m_light_box{nullptr};
        Widget* m_dark_box{nullptr};

        std::unique_ptr<ConfigManager> m_config;
        std::string m_variant{"dark"};
        bool m_is_loading{false};
    };
}
