#pragma once
#include <atomic>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Widget.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <views/BluetoothView/BluetoothDevice.hpp>

namespace horizon::preferences
{
    class BluetoothView : public horizon::Widget
    {
    public:
        BluetoothView();
        ~BluetoothView() override;

        void refresh_devices();

    private:
        void setup_ui();
        std::vector<BluetoothDevice> get_paired_devices();
        void monitor_loop();
        void on_device_selected(const BluetoothDevice &device);
        void on_sync_new_clicked();
        void on_connect_clicked();
        void toggle_connection();

        horizon::Label *m_title_label{nullptr};
        horizon::TableView<BluetoothDevice> *m_table_view{nullptr};
        horizon::Button<horizon::SolidObject> *m_connect_button{nullptr};
        horizon::Button<horizon::SolidObject> *m_sync_button{nullptr};
        horizon::Button<horizon::SolidObject> *m_refresh_button{nullptr};

        std::unique_ptr<horizon::dbusutils::DbusHelper> m_dbus;
        BluetoothDevice m_selected_device;
        bool m_initialized{false};
        bool m_dialog_open{false};

        std::thread m_monitor_thread;
        std::atomic<bool> m_stop_monitor{false};
        size_t m_refresh_timer_id{0};
    };
} // namespace horizon::preferences
