#pragma once

#include "horizon/CompositorAppInterface.hpp"
#include "horizon/IpcClient.hpp"
#include <memory>

namespace horizon
{
    class Application;

    /**
     * @class WayfireAppAdapter
     * @brief Implementation of CompositorAppInterface for Wayfire.
     * Currently utilizes the Horizon session IPC and Foreign Toplevels.
     */
    class WayfireAppAdapter : public CompositorAppInterface
    {
    public:
        WayfireAppAdapter(Application *app);
        ~WayfireAppAdapter() override = default;

        std::vector<ApplicationInfo> get_running_applications() override;

    private:
        void setup_ipc();
        void merge_and_notify();
        void handle_ipc_message(const std::string &msg);
        void handle_foreign_update(AppListEventContext &ctx);

        std::unique_ptr<IpcClient> m_ipc_client;
        std::vector<ApplicationInfo> m_ipc_apps;
        std::vector<ApplicationInfo> m_foreign_apps;
    };
} // namespace horizon
