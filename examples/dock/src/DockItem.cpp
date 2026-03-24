#include "DockItem.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{

    DockItem::DockItem(WaylandLayerWindow *app, const std::string &icon_name, bool is_wayfire)
        : _app(app), _is_wayfire(is_wayfire)
    {
        set_application_recursive(app);
        set_icon_name(icon_name);
        set_icon_size(48);
        set_margin(5);
        set_fixed_size(48 + margin() * 2);
    }

    void DockItem::set_app_info(const ApplicationInfo &info)
    {
        _pid = info.pid;
        _app_id = info.app_id;
        _is_minimized = info.is_minimized;
        _is_running = true;
        _instance_id = info.instance_id;

        if (!info.icon.empty())
            set_icon_name(info.icon);
        else if (icon_name().empty())
            set_icon_name(info.app_id);

        setup_running_behavior();
        invalidate();
    }

    void DockItem::set_pinned_data(const std::string &run_id)
    {
        _run_id = run_id;
        _is_running = false;
        setup_pinned_behavior();
        invalidate();
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
        when_click.connect(
            [this](MouseButtonEventContext &ctx)
            {
                auto *ca = _app->compositor_apps();
                if (!ca || _app_id.empty())
                    return;

                if (_is_minimized)
                {
                    if (_instance_id != 0)
                        ca->activate_instance(_instance_id);
                    else
                        ca->activate(_app_id);
                }
                else
                {
                    if (_instance_id != 0)
                        ca->minimize_instance(_instance_id);
                    else
                        ca->minimize(_app_id);
                }
            });
    }

    void DockItem::setup_pinned_behavior()
    {
        when_click.disconnect_all();
        when_click.connect(
            [this](MouseButtonEventContext &ctx)
            {
                LOG_INFO << "[DOCK] Requesting to run app: " << _run_id;
                ApplicationLauncher::launch(_run_id);
            });
    }

    void DockItem::draw(GraphicsContext &ctx)
    {
        Icon::draw(ctx);

        if (_is_running)
        {
            int indicator_size = 4;
            int x = m_start_draw_x + m_available_draw_width / 2;
            int y = m_start_draw_y + m_available_draw_height - 6;

            // Draw a glowing blue dot
            Color center_color("#00AAFF");
            Color edge_color("#00AAFF00");

            ctx.fillGradientCircle(x, y, indicator_size, center_color, edge_color, GradientDirection::Radial);
            ctx.fillCircle(x, y, indicator_size / 2); // Core of the dot
        }
    }

} // namespace horizon
