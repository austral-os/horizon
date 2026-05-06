#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct ext_idle_notifier_v1;
struct ext_idle_notification_v1;

namespace horizon
{
    /**
     * @class IdleManager
     * @brief Manages system-wide idle notifications via Wayland ext-idle-notify-v1.
     */
    class IdleManager
    {
    public:
        IdleManager();
        ~IdleManager();

        /**
         * @brief Initializes the Wayland connection and binds to the idle notifier.
         * @return true if successful, false otherwise.
         */
        bool init();

        /**
         * @brief Stops the idle manager and cleans up resources.
         */
        void stop();

        /**
         * @brief Registers a timeout for idle notification.
         * @param timeout_ms The inactivity period in milliseconds.
         * @param callback Function called with true when idle, false when resumed.
         * @return A handle that can be used to remove the timeout later.
         */
        void* add_idle_timeout(uint32_t timeout_ms, std::function<void(bool)> callback);

        /**
         * @brief Removes a previously registered idle timeout.
         * @param handle The handle returned by add_idle_timeout.
         */
        void remove_idle_timeout(void* handle);

        // Static handlers (Wayland callbacks)
        static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version);
        static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name);
        static void idle_handle_idled(void *data, struct ext_idle_notification_v1 *notification);
        static void idle_handle_resumed(void *data, struct ext_idle_notification_v1 *notification);

    private:
        void run_loop();
        
        struct Notification
        {
            struct ext_idle_notification_v1* obj;
            std::function<void(bool)> callback;
            bool is_idle{false};
            IdleManager* manager;
        };

        struct wl_display* m_display{nullptr};
        struct wl_registry* m_registry{nullptr};
        struct ext_idle_notifier_v1* m_notifier{nullptr};
        struct wl_seat* m_seat{nullptr};

        std::thread m_thread;
        std::atomic<bool> m_running{false};
        std::mutex m_mutex;
        std::vector<std::unique_ptr<Notification>> m_notifications;
    };
}
