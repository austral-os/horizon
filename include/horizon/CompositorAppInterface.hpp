#pragma once

#include "horizon/EventsManager.hpp"
#include <string>
#include <vector>

struct zwlr_foreign_toplevel_handle_v1;

namespace horizon
{
    /**
     * @struct ApplicationInfo
     * @brief Information about a running application.
     */
    struct ApplicationInfo
    {
        std::string app_id;
        std::string title;
        std::string icon;
        int pid{-1};
        struct zwlr_foreign_toplevel_handle_v1 *handle{nullptr};
        bool is_active{false};
        bool is_minimized{false};
        bool show_in_dock{true};
    };

    /**
     * @class AppListEventContext
     * @brief Context for application list update events.
     */
    class AppListEventContext : public EventContext
    {
    public:
        std::vector<ApplicationInfo> apps;
    };

    /**
     * @class CompositorAppInterface
     * @brief Interface for communicating with the compositor about running applications.
     */
    class CompositorAppInterface
    {
    public:
        virtual ~CompositorAppInterface() = default;

        /**
         * @brief Get a list of currently running applications.
         * @return A vector of ApplicationInfo objects.
         */
        virtual std::vector<ApplicationInfo> get_running_applications() = 0;

        /**
         * @brief Signal emitted when the application list or state changes.
         */
        EventsManager<AppListEventContext> when_update;

        // Instance-based management (Thread-safe Wayland handles)
        virtual void activate_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) = 0;
        virtual void minimize_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) = 0;
        virtual void toggle_fullscreen_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) = 0;
        virtual void close_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) = 0;
        virtual void set_instance_rectangle(struct zwlr_foreign_toplevel_handle_v1 *handle, int x, int y, int width, int height) = 0;

        // LEGACY - To be removed after full migration
        virtual void activate(const std::string &app_id) {}
        virtual void minimize(const std::string &app_id) {}
        virtual void toggle_fullscreen(const std::string &app_id) {}
        virtual void close(const std::string &app_id) {}
    };
} // namespace horizon
