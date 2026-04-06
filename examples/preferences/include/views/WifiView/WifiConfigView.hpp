#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/SolidObject.hpp>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include <views/WifiView/WifiConnectDialog.hpp>

namespace horizon::preferences
{
    struct WifiNetwork
    {
        std::string ssid;
        std::string security;
        std::string path;
        int signal;
        bool connected;
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
        std::string get_active_ssid();
        void monitor_loop();
        void on_network_selected(const WifiNetwork& network);
        void on_connect_clicked();
        void disconnect_selected();

        Label* m_title_label{nullptr};
        TableView<WifiNetwork>* m_table_view{nullptr};
        Button<SolidObject>* m_connect_button{nullptr};
        Button<SolidObject>* m_refresh_button{nullptr};
        Checkbox<AquaObject>* m_remember_checkbox{nullptr};
        Label* m_active_network_label{nullptr};
        
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::vector<WifiDevice> m_scan_devices;
        WifiNetwork m_selected_network;
        size_t m_refresh_timer_id{0};
        bool m_initialized{false};
        bool m_dialog_open{false};
        
        std::thread m_monitor_thread;
        std::atomic<bool> m_stop_monitor{false};
    };
}
