#include "DockItem.hpp"
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

    void DockItem::set_app_data(const nlohmann::json &app_j)
    {
        _pid = app_j.value("pid", -1);
        _app_id = app_j.value("app_id", "");
        _is_minimized = app_j.value("is_minimized", false);
        _is_running = true;
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
                if (ctx.button == 274) // BTN_MIDDLE
                {
                    send_sig("close");
                    return;
                }

                if (_is_minimized)
                {
                    if (_is_wayfire && !_app_id.empty())
                    {
                        _app->w_surface()->restore_foreign_app(_app_id);
                    }
                    else
                    {
                        _app->w_surface()->request_activation_token([this](const std::string &token)
                                                                    { send_sig("restore", token); },
                                                                    ctx.serial);
                    }
                }
                else
                {
                    send_sig("minimize");
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
                    _app->send_remote_signal(-1, "run_app", _run_id);
                }
            });
    }

} // namespace horizon
