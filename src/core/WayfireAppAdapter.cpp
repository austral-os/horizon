#include "horizon/WayfireAppAdapter.hpp"
#include "horizon/WaylandWindow.hpp"
#include <horizon/Logger.hpp>

namespace horizon
{
    WayfireAppAdapter::WayfireAppAdapter(WaylandWindow *app) : m_app(app)
    {
        if (m_app)
        {
            m_app->when_foreign_update.connect([this](AppListEventContext &ctx)
                                               { handle_foreign_update(ctx); });
        }
    }

    std::vector<ApplicationInfo> WayfireAppAdapter::get_running_applications()
    {
        return m_foreign_apps;
    }

    void WayfireAppAdapter::close(const std::string &app_id)
    {
        // For compatibility, we check for all instances and close them
        for (const auto &info : m_foreign_apps)
        {
            if (info.app_id == app_id && info.handle != nullptr)
            {
                close_instance(info.handle);
            }
        }
    }

    void WayfireAppAdapter::activate_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->activate_foreign_instance(handle);
    }

    void WayfireAppAdapter::minimize_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->minimize_foreign_instance(handle);
    }

    void WayfireAppAdapter::toggle_fullscreen_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->toggle_fullscreen_foreign_instance(handle);
    }

    void WayfireAppAdapter::close_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->close_foreign_instance(handle);
    }

    void WayfireAppAdapter::handle_ipc_message(const std::string &msg) {}

    void WayfireAppAdapter::handle_foreign_update(AppListEventContext &ctx)
    {
        m_foreign_apps = ctx.apps;
        when_update.run(ctx);
    }
} // namespace horizon
