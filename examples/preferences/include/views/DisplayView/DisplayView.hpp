#pragma once
#include <ConfigSection.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/SystemInfo.hpp>
#include <views/DisplayView/IDisplayAdapter.hpp>
#include <views/DisplayView/DisplayDevices.hpp>

namespace horizon::preferences
{
    class DisplayView : public Widget, public ConfigSection
    {
    public:
        DisplayView();
        ~DisplayView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json &j) override;
        nlohmann::json to_json() const override;

    private:
        void on_monitor_selected(int index);
        void show_confirmation();
        void revert_settings();
        void save_config();

        Label* m_title_label{nullptr};
        DisplayDevices* m_display_devices{nullptr};
        
        TableView<MonitorMode>* m_res_table{nullptr};
        Combo* m_rotation_combo{nullptr};
        Combo* m_refresh_combo{nullptr};
        Checkbox<AquaObject>* m_native_res_checkbox{nullptr};
        Button<AquaObject>* m_apply_button{nullptr};
        std::unique_ptr<IDisplayAdapter> m_adapter;

        std::vector<MonitorInfo> m_monitors;
        std::vector<MonitorConfig> m_previous_configs;
        size_t m_confirmation_timer_id{0};
        int m_countdown{10};
        Widget* m_overlay{nullptr};
        Label* m_countdown_label{nullptr};

        int m_selected_monitor_idx = -1;
    };
}
