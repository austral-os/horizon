#pragma once

#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Notification.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include "NotificationManager.hpp"
#include <map>

namespace horizon::notifications
{
    class NotificationWindow : public WaylandLayerWindow
    {
    public:
        NotificationWindow(NotificationManager &manager);
        ~NotificationWindow();

    private:
        NotificationManager &m_manager;
        Widget* m_root_widget{nullptr};
        
        void rebuild_notifications();
    };
}
