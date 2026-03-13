#include "horizon/WayfireAppAdapter.hpp"
#include <horizon/Application.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{
    WayfireAppAdapter::WayfireAppAdapter(Application *app) : m_app(app)
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

    void WayfireAppAdapter::activate(const std::string &app_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->activate_foreign_app(app_id);
    }

    void WayfireAppAdapter::minimize(const std::string &app_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->minimize_foreign_app(app_id);
    }

    void WayfireAppAdapter::toggle_fullscreen(const std::string &app_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->toggle_fullscreen_foreign_app(app_id);
    }

    void WayfireAppAdapter::close(const std::string &app_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->close_foreign_app(app_id);
    }

    void WayfireAppAdapter::activate_instance(uintptr_t instance_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->activate_foreign_instance(instance_id);
    }

    void WayfireAppAdapter::minimize_instance(uintptr_t instance_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->minimize_foreign_instance(instance_id);
    }

    void WayfireAppAdapter::toggle_fullscreen_instance(uintptr_t instance_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->toggle_fullscreen_foreign_instance(instance_id);
    }

    void WayfireAppAdapter::close_instance(uintptr_t instance_id)
    {
        if (m_app && m_app->w_surface())
            m_app->w_surface()->close_foreign_instance(instance_id);
    }

    void WayfireAppAdapter::handle_ipc_message(const std::string &msg) {}

    void WayfireAppAdapter::handle_foreign_update(AppListEventContext &ctx)
    {
        m_foreign_apps = ctx.apps;
        when_update.run(ctx);
    }
} // namespace horizon
