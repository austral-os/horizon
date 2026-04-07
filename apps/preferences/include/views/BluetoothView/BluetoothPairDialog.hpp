#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/LoadingBar.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <views/BluetoothView/BluetoothDevice.hpp>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <memory>

namespace horizon::preferences
{
    class BluetoothPairDialog : public horizon::WaylandWindow
    {
    public:
        BluetoothPairDialog();
        ~BluetoothPairDialog() override;

    private:
        void setup_ui();
        void start_scanning();
        void stop_scanning();
        void monitor_discovery();
        void filter_devices(const std::string& query);
        void on_device_selected(const BluetoothDevice& device);
        void on_next_clicked();
        
        horizon::TextBox<horizon::TextPolicy>* m_search_box{nullptr};
        horizon::TableView<BluetoothDevice>* m_table_view{nullptr};
        horizon::LoadingBar* m_loading_bar{nullptr};
        horizon::Label* m_status_label{nullptr};
        horizon::Button<horizon::AquaObject>* m_next_btn{nullptr};
        horizon::Button<horizon::AquaObject>* m_cancel_btn{nullptr};

        std::unique_ptr<horizon::dbusutils::DbusHelper> m_dbus;
        std::vector<BluetoothDevice> m_discovered_devices;
        BluetoothDevice m_selected_device;
        
        std::thread m_scan_thread;
        std::atomic<bool> m_stop_scan{false};
        std::string m_adapter_path = "/org/bluez/hci0";
    };
}
