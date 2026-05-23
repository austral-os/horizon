#pragma once

#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/FileWatcher.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <thread>

namespace horizon::portal
{
    class PortalService : public dbusutils::DbusObject, public horizon::FileWatcher
    {
    public:
        explicit PortalService(dbusutils::DbusHelper& dbus);
        ~PortalService() override;

        DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) override;

        /**
         * @brief Execute queued tasks on the main thread.
         */
        void process_tasks();

        /**
         * @brief Check for configuration changes and emit signals if necessary.
         */
        void check_config_changes();

    protected:
        void on_file_changed() override;
        void post_watcher_task(std::function<void()> task) override;

    private:
        dbusutils::DbusHelper& m_dbus;
        std::unique_ptr<ConfigManager> m_config;
        
        std::mutex m_tasks_mutex;
        std::vector<std::function<void()>> m_tasks;

        // Caches the last known color scheme so we can emit changes
        uint32_t m_last_color_scheme;
        time_t m_last_config_mod_time;

        void handle_read(DBusMessage* msg);
        void handle_read_all(DBusConnection* conn, DBusMessage* msg);

        uint32_t get_current_color_scheme();
        time_t get_config_mtime();

        void handle_file_chooser(DBusConnection* conn, DBusMessage* msg, int mode);
    };

    class RequestObject : public dbusutils::DbusObject
    {
    public:
        RequestObject(dbusutils::DbusHelper& dbus, const std::string& path) 
            : m_dbus(dbus), m_path(path) {}
            
        DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) override;
        
        bool is_closed() const { return m_closed; }

    private:
        dbusutils::DbusHelper& m_dbus;
        std::string m_path;
        bool m_closed{false};
    };
}
