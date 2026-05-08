#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon-network/NetworkTypes.hpp>
#include <horizon-network/NetworkManager.hpp>

namespace horizon::preferences
{
    class AdvancedNetworkView : public Widget
    {
    public:
        AdvancedNetworkView(const network::DeviceDetails& device);
        ~AdvancedNetworkView() override = default;

    private:
        void setup_ui();
        std::unique_ptr<Widget> create_general_tab();
        std::unique_ptr<Widget> create_wifi_tab();
        std::unique_ptr<Widget> create_tcpip_tab();
        
        void on_apply_clicked();
        void update_tcpip_fields_visibility();

        network::DeviceDetails m_device;
        
        // UI Components
        Notebook* m_notebook{nullptr};
        
        // TCP/IP Fields
        Combo* m_ipv4_method_combo{nullptr};
        TextBoxBase* m_ip_input{nullptr};
        TextBoxBase* m_mask_input{nullptr};
        TextBoxBase* m_router_input{nullptr};
        TextBoxBase* m_dns_input{nullptr};
        
        // Wifi Fields (if applicable)
        TextBoxBase* m_ssid_input{nullptr};
        Combo* m_security_combo{nullptr};
        TextBoxBase* m_password_input{nullptr};
    };
}
