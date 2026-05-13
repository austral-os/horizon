#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Combo.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TableView.hpp>
#include <string>
#include <vector>

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
        void load_history();
        void save_history();
        
        Combo* m_protocol_combo{nullptr};
        TextBox<>* m_address_input{nullptr};
        TableView<std::string>* m_history_table{nullptr};
        std::vector<std::string> m_history;
    };
}
