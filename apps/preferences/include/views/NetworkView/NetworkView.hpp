#pragma once
#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/NetworkTypes.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Widget.hpp>

namespace horizon::preferences
{
    class NetworkView : public Widget
    {
    public:
        NetworkView();
        ~NetworkView() override = default;

    private:
        void setup_ui();
        void refresh_devices();
        void on_device_selected(const network::DeviceDetails &dev);

        // UI Components
        TableView<network::DeviceDetails> *m_device_table{nullptr};
        Widget *m_details_container{nullptr};

        // Detail components
        Label *m_status_label{nullptr};
        Label *m_description_label{nullptr};
        Combo *m_config_combo{nullptr};
        Label *m_ip_label{nullptr};
        Label *m_mask_label{nullptr};
        Label *m_router_label{nullptr};
        Label *m_dns_label{nullptr};
        Label *m_domains_label{nullptr};
    };
} // namespace horizon::preferences
