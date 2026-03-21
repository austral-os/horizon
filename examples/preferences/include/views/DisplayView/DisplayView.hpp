#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Combo.hpp>
#include <horizon/SystemInfo.hpp>
#include <views/DisplayView/DisplayDevices.hpp>

namespace horizon::preferences
{
    class DisplayView : public Widget
    {
    public:
        DisplayView();
        ~DisplayView() override = default;
    private:
        void on_monitor_selected(int index);

        Label* m_title_label{nullptr};
        DisplayDevices* m_display_devices{nullptr};
        
        TableView<MonitorMode>* m_res_table{nullptr};
        Combo* m_rotation_combo{nullptr};
        Combo* m_refresh_combo{nullptr};

        std::vector<MonitorInfo> m_monitors;
        int m_selected_monitor_idx = -1;
    };
}
