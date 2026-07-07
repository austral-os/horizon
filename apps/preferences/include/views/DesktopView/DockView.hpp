#pragma once

#include <horizon/ConfigSection.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Combo.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>

namespace horizon::preferences
{
    /**
     * @class DockView
     * @brief View for configuring the Dock appearance and behavior.
     */
    class DockView : public horizon::Widget, public ConfigSection
    {
    public:
        DockView();
        ~DockView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json &j) override;
        nlohmann::json to_json() const override;

    private:
        void save_config();

    private:
        horizon::Slider *m_size_slider{nullptr};
        horizon::Label *m_size_label{nullptr};
        horizon::Checkbox<horizon::AquaObject> *m_magnification_check{nullptr};
        horizon::Checkbox<horizon::AquaObject> *m_autohide_check{nullptr};
        horizon::Combo *m_position_combo{nullptr};
        horizon::Checkbox<horizon::AquaObject> *m_show_trash_check{nullptr};
        horizon::Checkbox<horizon::AquaObject> *m_show_downloads_check{nullptr};
        horizon::TextBox<horizon::IntegerPolicy> *m_downloads_count_input{nullptr};

        int m_icon_size{64};
        bool m_magnification_enabled{true};
        bool m_autohide_enabled{false};
        std::string m_position{"bottom"};
        bool m_show_trash{true};
        bool m_show_downloads{true};
        int m_downloads_items_count{9};
        
        nlohmann::json m_config_data;

        std::unique_ptr<ConfigManager> m_config;
    };
} // namespace horizon::preferences
