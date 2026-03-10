#include "horizon/WayfireAppAdapter.hpp"
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <map>

namespace horizon
{
    WayfireAppAdapter::WayfireAppAdapter(Application *app)
    {
        setup_ipc();
        if (app)
        {
            app->when_foreign_update.connect([this](AppListEventContext &ctx)
                                             { handle_foreign_update(ctx); });
        }
    }

    std::vector<ApplicationInfo> WayfireAppAdapter::get_running_applications()
    {
        // Combined list
        std::map<std::string, ApplicationInfo> merged;

        // Add foreign apps first
        for (const auto &app : m_foreign_apps)
        {
            merged[app.app_id] = app;
        }

        // Add/Override with IPC apps
        for (const auto &app : m_ipc_apps)
        {
            if (merged.count(app.app_id))
            {
                auto &existing = merged[app.app_id];
                if (!app.icon.empty() && app.icon != "application-x-executable")
                    existing.icon = app.icon;
                if (!app.title.empty())
                    existing.title = app.title;
                existing.show_in_dock = app.show_in_dock;
                existing.pid = app.pid;
            }
            else
            {
                merged[app.app_id] = app;
            }
        }

        std::vector<ApplicationInfo> result;
        for (auto const &[id, info] : merged)
        {
            result.push_back(info);
        }
        return result;
    }

    void WayfireAppAdapter::setup_ipc()
    {
        m_ipc_client = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
        m_ipc_client->subscribe("{\"type\": \"subscribe\"}",
                                [this](const std::string &msg) { handle_ipc_message(msg); });

        // Request initial list
        std::string response;
        if (m_ipc_client->send("{\"type\": \"get_apps\"}", response))
        {
            handle_ipc_message(response);
        }
    }

    void WayfireAppAdapter::merge_and_notify()
    {
        AppListEventContext ctx;
        ctx.apps = get_running_applications();
        when_update.run(ctx);
    }

    void WayfireAppAdapter::handle_ipc_message(const std::string &msg)
    {
        try
        {
            auto j = nlohmann::json::parse(msg);
            std::string type = j.value("type", "");

            if (type == "app_list_updated" || type == "apps_list")
            {
                auto apps_j = j.at("apps");
                std::vector<ApplicationInfo> new_apps;

                for (const auto &app_j : apps_j)
                {
                    ApplicationInfo info;
                    info.app_id = app_j.value("app_id", "");
                    info.title = app_j.value("title", app_j.value("name", ""));
                    info.icon = app_j.value("icon", "application-x-executable");
                    info.pid = app_j.value("pid", -1);
                    info.is_active = app_j.value("is_active", false);
                    info.is_minimized = app_j.value("is_minimized", false);
                    info.show_in_dock = app_j.value("show_in_dock", true);
                    new_apps.push_back(info);
                }

                m_ipc_apps = new_apps;
                merge_and_notify();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "WayfireAppAdapter: Error parsing IPC message: " << e.what();
        }
    }

    void WayfireAppAdapter::handle_foreign_update(AppListEventContext &ctx)
    {
        m_foreign_apps = ctx.apps;
        merge_and_notify();
    }
} // namespace horizon
