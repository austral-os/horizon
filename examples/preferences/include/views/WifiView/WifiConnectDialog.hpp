#pragma once

#include <horizon/Window.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/EventsManager.hpp>
#include <string>
#include <vector>
#include <memory>

namespace horizon::preferences
{
    struct WifiDevice
    {
        std::string name;
        std::string path;
    };

    class WifiConnectDialog : public WaylandWindow
    {
    public:
        WifiConnectDialog(const std::string& ssid, const std::string& ap_path, const std::vector<WifiDevice>& devices);
        ~WifiConnectDialog() override = default;

        EventsManager<bool> when_finished;

    private:
        void setup_ui(const std::string& ssid);
        void perform_connection_async(const std::string& password, const std::string& device_path);

        std::string m_ssid;
        std::string m_ap_path;
        std::vector<WifiDevice> m_devices;

        Combo *m_device_combo{nullptr};
        TextBoxBase *m_password_input{nullptr};
        Button<AquaObject> *m_accept_btn{nullptr};
        Button<AquaObject> *m_cancel_btn{nullptr};
        Label *m_status_label{nullptr};

        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
    };
}
