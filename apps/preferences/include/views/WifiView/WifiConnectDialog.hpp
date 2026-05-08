#pragma once

#include <horizon/Window.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/LoadingBar.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon-network/WirelessDevice.hpp>
#include <horizon-network/NetworkTypes.hpp>
#include <string>
#include <vector>
#include <memory>

namespace horizon::preferences
{
    class WifiConnectDialog : public WaylandWindow
    {
    public:
        WifiConnectDialog(std::shared_ptr<network::WirelessDevice> device, const network::WifiNetwork& network);
        ~WifiConnectDialog() override = default;

        EventsManager<bool> when_finished;

    private:
        void setup_ui();
        void perform_connection_async(const std::string& password);

        std::shared_ptr<network::WirelessDevice> m_device;
        network::WifiNetwork m_network;

        TextBoxBase *m_password_input{nullptr};
        Button<AquaObject> *m_accept_btn{nullptr};
        Button<AquaObject> *m_cancel_btn{nullptr};
        Label *m_status_label{nullptr};
        LoadingBar *m_loading_bar{nullptr};
    };
}
