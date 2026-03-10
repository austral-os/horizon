#pragma once

#include "horizon/CompositorAppInterface.hpp"
#include "horizon/IpcClient.hpp"
#include <memory>

namespace horizon
{
    class Application;

    /**
     * @class LabwcAppAdapter
     * @brief Implementation of CompositorAppInterface for Labwc using IPC and Foreign Toplevels.
     */
    class LabwcAppAdapter : public CompositorAppInterface
    {
    public:
        LabwcAppAdapter(Application *app);
        ~LabwcAppAdapter() override = default;

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
