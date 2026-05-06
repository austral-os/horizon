#pragma once
#include <horizon/ConfigSection.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>

namespace horizon::preferences
{
    class PowerView : public Widget, public ConfigSection
    {
    public:
        PowerView();
        ~PowerView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json &j) override;
        nlohmann::json to_json() const override;

    private:
        std::unique_ptr<Widget> create_power_page(bool is_battery);
        bool has_battery() const;
        void save_config();
        void apply_system_settings();
        void apply_brightness(int value);
        void apply_power_profile();
        bool is_on_ac() const;

        Label* m_title_label{nullptr};
        Notebook* m_notebook{nullptr};

        // UI Controls for state management
        struct PageControls {
            Combo* profile_combo{nullptr};
            Slider* brightness_slider{nullptr};
            Label* brightness_label{nullptr};
            Checkbox<AquaObject>* brightness_check{nullptr};
            Combo* dim_combo{nullptr};
            Combo* turn_off_combo{nullptr};
            Slider* charge_slider{nullptr}; // Only for battery
            Label* charge_label{nullptr};   // Only for battery
        };

        PageControls m_ac_controls;
        PageControls m_battery_controls;

        std::unique_ptr<ConfigManager> m_config;
        bool m_is_loading{false};
    };
}
