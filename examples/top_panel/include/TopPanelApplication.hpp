#pragma once

#include <horizon/IpcClient.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TopPanelWidget;

class TopPanelApplication : public horizon::WaylandLayerWindow
{
public:
    TopPanelApplication();
    virtual ~TopPanelApplication() = default;

    void run_panel();

private:
    void setup_window();
    void setup_ipc();
    void process_messages();

    TopPanelWidget *m_root_widget;
    std::unique_ptr<horizon::IpcClient> m_ipc_client;

    std::mutex m_queue_mutex;
    std::vector<std::string> m_pending_messages;
};
