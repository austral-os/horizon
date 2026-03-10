#include "DockItem.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{

    DockItem::DockItem(LayerApplication *app, const std::string &icon_name, bool is_wayfire)
        : _app(app), _is_wayfire(is_wayfire)
    {
        set_icon_name(icon_name);
        set_icon_size(48);
        set_margin(5);
    }

    void DockItem::set_app_info(const ApplicationInfo &info)
    {
        _pid = info.pid;
        _app_id = info.app_id;
        _is_minimized = info.is_minimized;
        _is_running = true;

        if (!info.icon.empty())
            set_icon_name(info.icon);
        else if (icon_name().empty())
            set_icon_name(info.app_id);

        setup_running_behavior();
    }

    void DockItem::set_pinned_data(const std::string &run_id)
    {
        _run_id = run_id;
        _is_running = false;
        setup_pinned_behavior();
    }

    void DockItem::send_sig(const std::string &sig_name, const std::string &token)
    {
        if (_pid == -1)
            return;
        try
        {
            nlohmann::json sig;
            sig["type"] = "send_signal";
            sig["target_pid"] = _pid;
            sig["signal"] = sig_name;
            if (!token.empty())
                sig["token"] = token;

            IpcClient client("/tmp/horizon_session.sock");
            client.send(sig.dump());
        }
        catch (...)
        {
        }
    }

    void DockItem::setup_running_behavior()
    {
        when_mouse_press.disconnect_all();
        when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                auto *ca = _app->compositor_apps();
                if (!ca || _app_id.empty())
                    return;

                if (ctx.button == 274) // BTN_MIDDLE
                {
                    ca->close(_app_id);
                    return;
                }

                if (_is_minimized)
                {
                    ca->activate(_app_id);
                }
                else
                {
                    ca->minimize(_app_id);
                }
            });
    }

    void DockItem::setup_pinned_behavior()
    {
        when_mouse_press.disconnect_all();
        when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 272) // BTN_LEFT
                {
                    LOG_INFO << "[DOCK] Requesting to run app: " << _run_id;
                    ApplicationLauncher::launch(_run_id);
                }
            });
    }

} // namespace horizon
