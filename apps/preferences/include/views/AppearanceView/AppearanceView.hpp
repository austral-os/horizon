#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Slider.hpp>
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
        void load_compositor_config();
        void set_compositor(const std::string &comp);

        Label* m_title_label{nullptr};
        Widget* m_light_box{nullptr};
        Widget* m_dark_box{nullptr};
        Checkbox<AquaObject>* m_compositor_checkbox{nullptr};
        Label* m_restart_hint_label{nullptr};

        Slider* m_panel_opacity_slider{nullptr};
        Label* m_panel_opacity_label{nullptr};
        Slider* m_menu_opacity_slider{nullptr};
        Label* m_menu_opacity_label{nullptr};
        Slider* m_application_opacity_slider{nullptr};
        Label* m_application_opacity_label{nullptr};

        std::unique_ptr<ConfigManager> m_config;
        std::string m_variant{"dark"};
        std::string m_compositor{"labwc"};
        float m_panel_opacity{1.0f};
        float m_menu_opacity{1.0f};
        float m_application_opacity{1.0f};
        bool m_is_loading{false};
    };
}
