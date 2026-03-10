#pragma once

#include "horizon/CompositorAppInterface.hpp"
#include <vector>

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

        void activate(const std::string &app_id) override;
        void minimize(const std::string &app_id) override;
        void close(const std::string &app_id) override;

    private:
        void setup_ipc();
        void merge_and_notify();
        void handle_ipc_message(const std::string &msg);
        void handle_foreign_update(AppListEventContext &ctx);

        Application *m_app;
        std::vector<ApplicationInfo> m_foreign_apps;
    };
} // namespace horizon
