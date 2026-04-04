#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <vector>
#include <string>

#include <views/WifiView/WifiConnectDialog.hpp>

namespace horizon::preferences
{
    struct WifiNetwork
    {
        std::string ssid;
        std::string security;
        std::string path;
    };

    class WifiConfigView : public Widget
    {
    public:
        WifiConfigView();
        ~WifiConfigView() override;

        void refresh_networks();

    private:
        void setup_ui();
        std::vector<WifiNetwork> scan_networks();
        void on_network_selected(const WifiNetwork& network);

        Label* m_title_label{nullptr};
        TableView<WifiNetwork>* m_table_view{nullptr};
        Button<AquaObject>* m_add_button{nullptr};
        Button<AquaObject>* m_remove_button{nullptr};
        Button<AquaObject>* m_refresh_button{nullptr};
        Checkbox<AquaObject>* m_remember_checkbox{nullptr};
        
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::vector<WifiDevice> m_scan_devices;
        size_t m_refresh_timer_id{0};
    };
}
