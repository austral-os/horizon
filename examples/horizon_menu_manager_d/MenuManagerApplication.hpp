#pragma once

#include <horizon/Application.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace horizon
{
    class MenuManagerApplication : public Application
    {
    public:
        MenuManagerApplication();
        ~MenuManagerApplication() override;

    private:
        void setup_window();
        void setup_ipc();
        void setup_event_handlers();
        void process_messages();
        void hide_daemon();

        WaylandLayerWindow *m_window{nullptr};
        Widget *m_root_ptr{nullptr};
        
        std::unique_ptr<IpcClient> m_ipc_client;
        MessageManager m_message_manager;
        std::unique_ptr<RequestRouter> m_router;

        bool m_menu_visible{false};
        std::mutex m_queue_mutex;
        std::vector<std::string> m_pending_messages;
    };
} // namespace horizon
