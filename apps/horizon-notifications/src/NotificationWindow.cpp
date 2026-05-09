#include "NotificationWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Logger.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

namespace horizon::notifications
{
    NotificationWindow::NotificationWindow(NotificationManager &manager)
        : WaylandLayerWindow("horizon-notifications", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, true), m_manager(manager)
    {
        // Set a default size before initializing the layer surface
        // to avoid "width 0" errors in some compositors.
        set_size(400, 100);
        
        set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        set_exclusive_zone(0); 
        
        // Create root widget
        auto root = std::make_unique<Widget>();
        m_root_widget = root.get();
        m_root_widget->set_background_color(Color{0.0f, 0.0f, 0.0f, 0.0f});
        m_root_widget->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_root_widget->set_spacing(10);
        m_root_widget->set_margin(10);
        
        set_root(std::move(root));
        
        m_manager.set_on_changed_callback([this]() {
            post_task([this]() {
                rebuild_notifications();
            });
        });
        
        rebuild_notifications();
        set_visible(true);
    }

    NotificationWindow::~NotificationWindow()
    {
    }

    void NotificationWindow::rebuild_notifications()
    {
        if (!m_root_widget) return;
        
        LOG_INFO << "[WINDOW] Rebuilding notifications...";

        m_root_widget->clear_children();
        
        const auto& data_list = m_manager.active_notifications();
        
        for (const auto& data : data_list)
        {
            auto notif = std::make_unique<horizon::Notification>();
            notif->set_notification(data.icon_name, data.summary + "\n" + data.body);
            notif->set_fixed_width(380);
            
            uint32_t id = data.id;
            
            // Add a timer to remove it
            add_timer(data.timeout, [this, id]() {
                m_manager.remove_notification(id);
            });
            
            m_root_widget->add_child(std::move(notif));
        }
        
        // Resize window height based on content
        int total_h = 0;
        for (const auto& child : m_root_widget->children()) {
            total_h += child->preferred_height(380) + 10;
        }
        if (total_h == 0) total_h = 1;
        total_h += 20; // Padding
        
        set_size(400, total_h);
        invalidate();
    }
}
