#pragma once

#include "horizon/CompositorAppInterface.hpp"
#include <vector>

namespace horizon
{
    class WaylandWindow;

    /**
     * @class WayfireAppAdapter
     * @brief Implementation of CompositorAppInterface for Wayfire.
     * Currently utilizes the Horizon session IPC and Foreign Toplevels.
     */
    class WayfireAppAdapter : public CompositorAppInterface
    {
    public:
        WayfireAppAdapter(WaylandWindow *app);
        ~WayfireAppAdapter() override = default;

        std::vector<ApplicationInfo> get_running_applications() override;

        // Convenience methods
        void close(const std::string &app_id) override;

        void activate_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) override;
        void minimize_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) override;
        void toggle_fullscreen_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) override;
        void close_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) override;

    private:
        void setup_ipc();
        void merge_and_notify();
        void handle_ipc_message(const std::string &msg);
        void handle_foreign_update(AppListEventContext &ctx);

        WaylandWindow *m_app;
        std::vector<ApplicationInfo> m_foreign_apps;
    };
} // namespace horizon
