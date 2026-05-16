#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/WirelessDevice.hpp>
#include <vector>
#include <string>
#include <memory>
#include <atomic>

namespace horizon::preferences
{
    class WifiConfigView : public Widget
    {
    public:
        WifiConfigView();
        ~WifiConfigView() override;

        void refresh_networks();

    private:
        void setup_ui();
        void on_network_selected(const network::WifiNetwork& network);
        void on_connect_clicked();
        void disconnect_selected();

        Label* m_title_label{nullptr};
        TableView<network::WifiNetwork>* m_table_view{nullptr};
        Button<SolidObject>* m_connect_button{nullptr};
        Button<SolidObject>* m_refresh_button{nullptr};
        Label* m_active_network_label{nullptr};
        
        std::shared_ptr<network::WirelessDevice> m_device;
        network::WifiNetwork m_selected_network;
        size_t m_refresh_timer_id{0};
        size_t m_state_changed_connection_id{0};
        bool m_initialized{false};
        bool m_dialog_open{false};
        std::shared_ptr<std::atomic<bool>> m_alive{std::make_shared<std::atomic<bool>>(true)};
    };
}
