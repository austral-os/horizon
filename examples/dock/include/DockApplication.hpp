#pragma once

#include <horizon/ClientMenu.hpp>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LayerApplication.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/RequestRouter.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace horizon
{

    class DockShelf;

    struct PinnedApp
    {
        std::string app_id;
        std::string name;
        std::string icon;
        std::string run_id;
    };

    class DockApplication : public LayerApplication
    {
    public:
        DockApplication();
        ~DockApplication() override;

        CompositorAppInterface *compositor_apps() override;

        // Shows the dock context menu at the given screen position.
        void show_dock_context_menu(int x, int y);

    private:
        void detect_environment();
        void setup_ui();
        void setup_ipc();
        void setup_context_menu_ipc();
        void update_dock(const std::vector<ApplicationInfo> &apps);

        bool _is_wayfire = false;
        DockShelf *_shelf_ptr = nullptr;
        std::unique_ptr<CompositorAppInterface> _compositor_apps;
        static const std::vector<PinnedApp> PINNED_APPS;

        // Context menu IPC
        ClientMenu _client_menu;
        std::unique_ptr<IpcClient> _menu_ipc_client;
        MessageManager _message_manager;
        std::unique_ptr<RequestRouter> _router;
        std::mutex _queue_mutex;
        std::vector<std::string> _pending_messages;
    };

} // namespace horizon
