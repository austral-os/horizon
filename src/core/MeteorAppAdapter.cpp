#include "horizon/MeteorAppAdapter.hpp"
#include "horizon/WaylandWindow.hpp"
#include <horizon/Logger.hpp>

namespace horizon
{
    MeteorAppAdapter::MeteorAppAdapter(WaylandWindow *app) : m_app(app)
    {
        if (m_app)
        {
            m_app->when_foreign_update.connect([this](AppListEventContext &ctx)
                                               { handle_foreign_update(ctx); });
        }
    }

    std::vector<ApplicationInfo> MeteorAppAdapter::get_running_applications()
    {
        return m_foreign_apps;
    }

    void MeteorAppAdapter::close(const std::string &app_id)
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

    void MeteorAppAdapter::activate_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->activate_foreign_instance(handle);
    }

    void MeteorAppAdapter::minimize_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->minimize_foreign_instance(handle);
    }

    void MeteorAppAdapter::toggle_fullscreen_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->toggle_fullscreen_foreign_instance(handle);
    }

    void MeteorAppAdapter::close_instance(struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->close_foreign_instance(handle);
    }

    void MeteorAppAdapter::handle_ipc_message(const std::string &msg) {}

    void MeteorAppAdapter::handle_foreign_update(AppListEventContext &ctx)
    {
        m_foreign_apps = ctx.apps;
        when_update.run(ctx);
    }
} // namespace horizon
