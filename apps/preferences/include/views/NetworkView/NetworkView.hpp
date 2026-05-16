#pragma once
#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/NetworkTypes.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Widget.hpp>
#include <atomic>
#include <memory>

namespace horizon::preferences
{
    class NetworkView : public Widget
    {
    public:
        NetworkView();
        ~NetworkView() override;

        EventsManager<network::DeviceDetails> when_advanced_click;
        EventsManager<std::string> when_selection_changed;

        void select_device_by_path(const std::string &path);

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
        Label *m_connection_name_label{nullptr};
        Label *m_ip_label{nullptr};
        Label *m_mask_label{nullptr};
        Label *m_router_label{nullptr};
        Label *m_dns_label{nullptr};

        network::DeviceDetails m_selected_device;
        size_t m_state_changed_connection_id{0};

        // Shared flag to cancel pending post_task callbacks after destruction
        std::shared_ptr<std::atomic<bool>> m_alive{std::make_shared<std::atomic<bool>>(true)};
    };
} // namespace horizon::preferences
