#pragma once

#include "horizon/EventsManager.hpp"
#include <string>
#include <vector>

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
     * @brief Interface for querying running applications from the compositor.
     */
    class CompositorAppInterface
    {
    public:
        virtual ~CompositorAppInterface() = default;

        /**
         * @brief Gets the current list of running applications.
         * @return A vector of ApplicationInfo.
         */
        virtual std::vector<ApplicationInfo> get_running_applications() = 0;

        /**
         * @brief Event fired when the application list is updated.
         */
        EventsManager<AppListEventContext> when_update;

        /**
         * @brief Activates/Restores an application.
         */
        virtual void activate(const std::string &app_id) = 0;

        /**
         * @brief Minimizes an application.
         */
        virtual void minimize(const std::string &app_id) = 0;

        /**
         * @brief Closes an application.
         */
        virtual void close(const std::string &app_id) = 0;
    };
} // namespace horizon
