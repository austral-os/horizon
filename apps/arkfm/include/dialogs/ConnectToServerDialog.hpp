#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Combo.hpp>
#include <horizon/TextBox.hpp>
#include <string>

namespace horizon::arkfm
{
    struct ConnectToServerEvent : public EventContext
    {
        std::string uri;
    };

    class ConnectToServerDialog : public WaylandWindow
    {
    public:
        ConnectToServerDialog();
        ~ConnectToServerDialog() override = default;
        
        EventsManager<ConnectToServerEvent> when_accepted;

    private:
        void handle_connect();
        
        Combo* m_protocol_combo{nullptr};
        TextBox<>* m_address_input{nullptr};
    };
}
