#include "horizon/IdleManager.hpp"
#include "horizon/Logger.hpp"
#include <protocols/ext-idle-notify-v1-client-protocol.h>
#include <wayland-client.h>
#include <cstring>
#include <poll.h>
#include <unistd.h>

namespace horizon
{
    static const struct wl_registry_listener registry_listener = {
        IdleManager::registry_handle_global,
        IdleManager::registry_handle_global_remove
    };

    static const struct ext_idle_notification_v1_listener notification_listener = {
        IdleManager::idle_handle_idled,
        IdleManager::idle_handle_resumed
    };

    IdleManager::IdleManager() {}

    IdleManager::~IdleManager()
    {
        stop();
    }

    bool IdleManager::init()
    {
        m_display = wl_display_connect(nullptr);
        if (!m_display)
        {
            LOG_ERROR << "[IdleManager] Failed to connect to Wayland display";
            return false;
        }

        m_registry = wl_display_get_registry(m_display);
        wl_registry_add_listener(m_registry, &registry_listener, this);

        // Roundtrip to get globals
        wl_display_roundtrip(m_display);

        if (!m_notifier)
        {
            LOG_ERROR << "[IdleManager] Compositor does not support ext-idle-notify-v1";
            return false;
        }

        m_running = true;
        m_thread = std::thread(&IdleManager::run_loop, this);

        return true;
    }

    void IdleManager::stop()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& n : m_notifications)
        {
            if (n->obj) ext_idle_notification_v1_destroy(n->obj);
        }
        m_notifications.clear();

        if (m_notifier) ext_idle_notifier_v1_destroy(m_notifier);
        if (m_registry) wl_registry_destroy(m_registry);
        if (m_display) wl_display_disconnect(m_display);
        
        m_notifier = nullptr;
        m_registry = nullptr;
        m_display = nullptr;
    }

    void* IdleManager::add_idle_timeout(uint32_t timeout_ms, std::function<void(bool)> callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto n = std::make_unique<Notification>();
        n->callback = callback;
        n->manager = this;
        n->obj = ext_idle_notifier_v1_get_idle_notification(m_notifier, timeout_ms, m_seat);
        
        ext_idle_notification_v1_add_listener(n->obj, &notification_listener, n.get());
        
        void* handle = n.get();
        m_notifications.push_back(std::move(n));
        
        return handle;
    }

    void IdleManager::remove_idle_timeout(void* handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_notifications.begin(); it != m_notifications.end(); ++it)
        {
            if (it->get() == handle)
            {
                ext_idle_notification_v1_destroy((*it)->obj);
                m_notifications.erase(it);
                break;
            }
        }
    }

    void IdleManager::run_loop()
    {
        struct pollfd fds[1];
        fds[0].fd = wl_display_get_fd(m_display);
        fds[0].events = POLLIN;

        while (m_running)
        {
            while (wl_display_prepare_read(m_display) != 0)
            {
                wl_display_dispatch_pending(m_display);
            }
            
            wl_display_flush(m_display);

            if (poll(fds, 1, 100) > 0)
            {
                if (fds[0].revents & POLLIN)
                {
                    wl_display_read_events(m_display);
                }
                else
                {
                    wl_display_cancel_read(m_display);
                }
            }
            else
            {
                wl_display_cancel_read(m_display);
            }

            wl_display_dispatch_pending(m_display);
        }
    }

    void IdleManager::registry_handle_global(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
    {
        auto* self = static_cast<IdleManager*>(data);
        if (strcmp(interface, "ext_idle_notifier_v1") == 0)
        {
            self->m_notifier = static_cast<ext_idle_notifier_v1*>(wl_registry_bind(registry, id, &ext_idle_notifier_v1_interface, 1));
        }
        else if (strcmp(interface, "wl_seat") == 0)
        {
            self->m_seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, 1));
        }
    }

    void IdleManager::registry_handle_global_remove(void*, struct wl_registry*, uint32_t) {}

    void IdleManager::idle_handle_idled(void *data, struct ext_idle_notification_v1 *)
    {
        auto* n = static_cast<Notification*>(data);
        n->is_idle = true;
        if (n->callback) n->callback(true);
    }

    void IdleManager::idle_handle_resumed(void *data, struct ext_idle_notification_v1 *)
    {
        auto* n = static_cast<Notification*>(data);
        n->is_idle = false;
        if (n->callback) n->callback(false);
    }
}
