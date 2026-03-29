#include "DockItem.hpp"
#include <horizon/DesktopEntry.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Matrix.hpp>

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

    void DockItem::add_instance(const ApplicationInfo &info)
    {
        _instances.push_back(info);
        _app_id = info.app_id;

        if (!info.icon.empty())
            set_icon_name(info.icon);
        else if (icon_name().empty() || icon_name() == _app_id)
        {
            std::string resolved_icon = DesktopEntry::get_icon_name(info.app_id);
            
            // Fallback for namespaced app_ids (e.g., org.gnome.Terminal -> Terminal)
            if (resolved_icon.empty() && info.app_id.find('.') != std::string::npos)
            {
                size_t last_dot = info.app_id.find_last_of('.');
                resolved_icon = DesktopEntry::get_icon_name(info.app_id.substr(last_dot + 1));
            }

            if (!resolved_icon.empty())
                set_icon_name(resolved_icon);
            else if (icon_name().empty())
                set_icon_name(info.app_id);
        }

        setup_running_behavior();
        invalidate();
    }

    void DockItem::set_pinned_data(const std::string &run_id)
    {
        _run_id = run_id;
        _instances.clear();
        setup_pinned_behavior();
        invalidate();
    }

    void DockItem::send_sig(const std::string &sig_name, const std::string &token)
    {
        if (_instances.empty() || _instances[0].pid == -1)
            return;
        try
        {
            nlohmann::json sig;
            sig["type"] = "send_signal";
            sig["target_pid"] = _instances[0].pid;
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
        when_click.disconnect_all();
        when_click.connect(
            [this](MouseButtonEventContext &ctx)
            {
                auto *ca = _app->compositor_apps();
                if (!ca || _instances.empty())
                    return;

                // Find the first minimized instance to activate
                for (const auto &info : _instances)
                {
                    if (info.is_minimized)
                    {
                        if (info.instance_id != 0)
                            ca->activate_instance(info.instance_id);
                        else
                            ca->activate(info.app_id);
                        return;
                    }
                }

                // If none are minimized, activate the first one
                const auto &info = _instances[0];
                if (info.instance_id != 0)
                    ca->activate_instance(info.instance_id);
                else
                    ca->activate(info.app_id);
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
        // Capture the icon + indicator into a group to get it as a texture
        // BUT we also want it to remain on the Cairo buffer for the main draw
        // (actually, we could draw it via OpenGL too, which is what we'll do for consistency)
        ctx.pushGroup();
        
        Icon::draw(ctx);
        if (is_running())
        {
            int indicator_size = 4;
            int x = m_start_draw_x + m_available_draw_width / 2;
            int y = m_start_draw_y + m_available_draw_height - 6;
            Color center_color("#00AAFF");
            Color edge_color("#00AAFF00");
            ctx.fillGradientCircle(x, y, indicator_size, center_color, edge_color, GradientDirection::Radial);
            ctx.fillCircle(x, y, indicator_size / 2);
        }
        
        uint32_t tex_id = 0;
        ctx.popGroupToTexture(tex_id, m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height);

        // 2. Draw the main icon (OpenGL)
        float mvp[16];
        Matrix::identity(mvp);
        
        // Ortho projection: maps [0, W]x[H, 0] to NDC [-1, 1]
        // Critical: Must use the ACTUAL window surface width/height.
        // m_app->width() returns the current surface size.
        Matrix::ortho(mvp, 0, (float)_app->width(), (float)_app->height(), 0, -1, 1);
        
        // In Horizon, m_start_draw_x/y are already cumulative (window-relative)
        // because each calculate_layout uses the parent's start position.
        float window_x = (float)m_start_draw_x;
        float window_y = (float)m_start_draw_y;

        float main_mvp[16];
        std::memcpy(main_mvp, mvp, 16 * sizeof(float));
        Matrix::translate(main_mvp, window_x + m_available_draw_width / 2.0f, 
                          window_y + m_available_draw_height / 2.0f, 0);
        // Correct Y-scale: OpenGL textures are usually flipped relative to screen space,
        // and our Ortho maps Top to 1.0, Bottom to -1.0. 
        // A scale of -h/2 maps v.y=1 (top of quad) to y=0 (top of area).
        Matrix::scale(main_mvp, m_available_draw_width / 2.0f, -m_available_draw_height / 2.0f, 1);
        
        ctx.drawTexture3D(tex_id, m_available_draw_width, m_available_draw_height, main_mvp, 1.0f, false);

        // 3. Draw Reflection (below)
        float refl_mvp[16];
        std::memcpy(refl_mvp, mvp, 16 * sizeof(float));
        
        float refl_height = m_available_draw_height * 0.4f; // 40% height for reflection
        
        // Reflection should meet main icon at its bottom
        Matrix::translate(refl_mvp, window_x + m_available_draw_width / 2.0f, 
                          window_y + m_available_draw_height + refl_height / 2.0f, 0);
        // Scale is positive to flip relative to the main icon's negative scale
        Matrix::scale(refl_mvp, m_available_draw_width / 2.0f, refl_height / 2.0f, 1);
        
        // Queue reflected draw with gradient
        // In our flipped quad, v_texcoord.y=1 is the 'top' (connected to original)
        // and v_texcoord.y=0 is the 'bottom' (farthest away).
        WaylandWindow::GLDrawCall refl_call;
        refl_call.texture_id = tex_id;
        std::memcpy(refl_call.mvp, refl_mvp, 16 * sizeof(float));
        refl_call.opacity = 0.5f;
        refl_call.delete_texture = true; // Delete after the last draw of this texture
        refl_call.use_scissor = false;
        refl_call.gradient_start = 0.0f; // Fade out at distant edge
        refl_call.gradient_end = 1.0f;   // Full at junction edge
        
        _app->queue_gl_draw(refl_call);
    }

} // namespace horizon
