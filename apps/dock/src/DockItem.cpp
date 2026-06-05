#include "DockItem.hpp"
#include "DockShelf.hpp"
#include <horizon/DesktopManager.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Matrix.hpp>

namespace horizon
{

    DockItem::DockItem(WaylandLayerWindow *app, CompositorAppInterface *compositor_apps, const std::string &icon_name, bool is_meteor)
        : _app(app), _compositor_apps(compositor_apps), _is_meteor(is_meteor)
    {
        set_application_recursive(app);
        set_icon_name(icon_name);
        set_icon_size(48);
        set_fixed_size(48 + margin() * 2);
        setup_drag_behavior();
    }

    void DockItem::add_instance(const ApplicationInfo &info)
    {
        _instances.push_back(info);
        if (_app_id.empty())
            _app_id = info.app_id;
        if (_name.empty())
            _name = info.title;

        if (!info.icon.empty())
            set_icon_name(info.icon);
        else if (icon_name().empty() || icon_name() == _app_id)
        {
            std::string resolved_icon = DesktopManager::get_icon_name(info.app_id);
            
            // Fallback for namespaced app_ids (e.g., org.gnome.Terminal -> Terminal)
            if (resolved_icon.empty() && info.app_id.find('.') != std::string::npos)
            {
                size_t last_dot = info.app_id.find_last_of('.');
                resolved_icon = DesktopManager::get_icon_name(info.app_id.substr(last_dot + 1));
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
                if (std::abs(ctx.x - _press_x) > 10 || std::abs(ctx.y - _press_y) > 10)
                    return;

                if (!_compositor_apps || _instances.empty())
                    return;

                // Debug log matching requirements
                const auto &first = _instances[0];
                LOG_INFO << "[DOCK-ITEM] Clicked app_id='" << _app_id 
                         << "' instances=" << _instances.size() 
                         << " handle=" << (void*)first.handle 
                         << " pid=" << first.pid;

                // 1. If there is a minimized window -> restore it
                for (const auto &info : _instances)
                {
                    if (info.is_minimized && info.handle != nullptr)
                    {
                        LOG_INFO << "[DOCK-ITEM] Restoring minimized instance: " << (void*)info.handle;
                        _compositor_apps->activate_instance(info.handle);
                        return;
                    }
                }

                // 2. Otherwise -> activate the first instance
                if (first.handle != nullptr) {
                    LOG_INFO << "[DOCK-ITEM] Activating first instance: " << (void*)first.handle;
                    _compositor_apps->activate_instance(first.handle);
                } else {
                    // Fallback to remote signal if handle is null
                    LOG_INFO << "[DOCK-ITEM] No handle available, falling back to PID signal: " << first.pid;
                    _app->send_remote_signal(first.pid, "activate");
                }
            });
    }

    void DockItem::setup_pinned_behavior()
    {
        when_click.disconnect_all();
        when_click.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (std::abs(ctx.x - _press_x) > 10 || std::abs(ctx.y - _press_y) > 10)
                    return;

                LOG_INFO << "[DOCK] Requesting to run app: " << _run_id;
                ApplicationLauncher::launch(_run_id);
            });
    }

    void DockItem::setup_drag_behavior()
    {
        when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
            _press_x = ctx.x;
            _press_y = ctx.y;
        });

        when_mouse_drag.connect([this](MouseMoveEventContext &ctx) {
            if (!_dragging && (std::abs(ctx.x - _press_x) > 10 || std::abs(ctx.y - _press_y) > 10)) {
                _dragging = true;
                
                // Notify parent DockShelf
                DockShelf* shelf = dynamic_cast<DockShelf*>(parent());
                if (shelf) {
                    shelf->start_drag(this, ctx.x, ctx.y);
                }
                
                invalidate();
            }
            
            if (_dragging) {
                DockShelf* shelf = dynamic_cast<DockShelf*>(parent());
                if (shelf) {
                    shelf->update_drag(ctx.x, ctx.y);
                }
            }
        });

        when_mouse_release.connect([this](MouseButtonEventContext &ctx) {
            if (_dragging) {
                _dragging = false;
                
                DockShelf* shelf = dynamic_cast<DockShelf*>(parent());
                if (shelf) {
                    shelf->end_drag();
                    // Do NOT access `this` or call `invalidate()` here, 
                    // because `end_drag()` may recreate the dock and delete this item.
                } else {
                    invalidate();
                }
            }
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

        if (_compositor_apps && is_running() && 
            (_last_rect_x != m_start_draw_x || _last_rect_y != m_start_draw_y || 
             _last_rect_w != m_available_draw_width || _last_rect_h != m_available_draw_height)) {
             
            _last_rect_x = m_start_draw_x;
            _last_rect_y = m_start_draw_y;
            _last_rect_w = m_available_draw_width;
            _last_rect_h = m_available_draw_height;
            
            for (const auto& info : _instances) {
                if (info.handle != nullptr) {
                    _compositor_apps->set_instance_rectangle(info.handle, _last_rect_x, _last_rect_y, _last_rect_w, _last_rect_h);
                }
            }
        }

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
        
        float opacity = _dragging ? 0.5f : 1.0f;
        ctx.drawTexture3D(tex_id, m_available_draw_width, m_available_draw_height, main_mvp, opacity, false);

        if (_dragging) {
            // If dragging, we don't draw the reflection (or keep it as it is)
            // Actually, we might want the DockShelf to handle the "floating" icon.
            // For now, let's keep it simple.
        }

        std::string position = "bottom";
        DockShelf* shelf = dynamic_cast<DockShelf*>(parent());
        if (shelf) {
            position = shelf->dock_position();
        }

        // 3. Draw Reflection
        float refl_mvp[16];
        std::memcpy(refl_mvp, mvp, 16 * sizeof(float));
        
        WaylandWindow::GLDrawCall refl_call;
        refl_call.texture_id = tex_id;
        refl_call.opacity = 0.5f;
        refl_call.delete_texture = true;
        refl_call.use_scissor = false;
        
        if (position == "left") {
            float refl_size = m_available_draw_width * 0.4f;
            Matrix::translate(refl_mvp, window_x - refl_size / 2.0f, 
                              window_y + m_available_draw_height / 2.0f, 0);
            Matrix::scale(refl_mvp, -refl_size / 2.0f, -m_available_draw_height / 2.0f, 1);
            
            refl_call.gradient_horizontal = true;
            refl_call.gradient_start = 1.0f; // Near icon
            refl_call.gradient_end = 0.0f;   // Far edge
        } else if (position == "right") {
            float refl_size = m_available_draw_width * 0.4f;
            Matrix::translate(refl_mvp, window_x + m_available_draw_width + refl_size / 2.0f, 
                              window_y + m_available_draw_height / 2.0f, 0);
            Matrix::scale(refl_mvp, -refl_size / 2.0f, -m_available_draw_height / 2.0f, 1);
            
            refl_call.gradient_horizontal = true;
            refl_call.gradient_start = 0.0f; // Far edge
            refl_call.gradient_end = 1.0f;   // Near icon
        } else {
            float refl_height = m_available_draw_height * 0.4f;
            Matrix::translate(refl_mvp, window_x + m_available_draw_width / 2.0f, 
                              window_y + m_available_draw_height + refl_height / 2.0f, 0);
            Matrix::scale(refl_mvp, m_available_draw_width / 2.0f, refl_height / 2.0f, 1);
            
            refl_call.gradient_horizontal = false;
            refl_call.gradient_start = 0.0f;
            refl_call.gradient_end = 1.0f;
        }
        
        std::memcpy(refl_call.mvp, refl_mvp, 16 * sizeof(float));
        _app->queue_gl_draw(refl_call);
    }

} // namespace horizon
