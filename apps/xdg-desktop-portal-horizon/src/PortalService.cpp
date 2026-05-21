#include "PortalService.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

using namespace horizon::dbusutils;

namespace horizon::portal
{
    static std::string get_config_path() {
        const char* homedir;
        if ((homedir = getenv("HOME")) == NULL) {
            homedir = getpwuid(getuid())->pw_dir;
        }
        return std::string(homedir) + "/.config/horizon/color-scheme.json";
    }

    PortalService::PortalService(DbusHelper& dbus)
        : m_dbus(dbus), m_last_color_scheme(0), m_last_config_mod_time(0)
    {
        m_config = std::make_unique<ConfigManager>(get_config_path());
        m_last_color_scheme = get_current_color_scheme();
        start_watching(get_config_path());
    }

    PortalService::~PortalService() {
        stop_watching();
    }

    uint32_t PortalService::get_current_color_scheme()
    {
        m_config->load();
        // 0: default, 1: prefer-dark, 2: prefer-light
        auto val = m_config->get_section("variant");
        
        if (val.is_string()) {
            std::string s = val.get<std::string>();
            if (s == "dark" || s == "prefer-dark") return 1;
            if (s == "light" || s == "prefer-light") return 2;
        }
        return 0; // Default fallback
    }

    void PortalService::process_tasks()
    {
        std::vector<std::function<void()>> tasks;
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            tasks = std::move(m_tasks);
        }
        for (auto& task : tasks) {
            task();
        }
    }

    void PortalService::post_watcher_task(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        m_tasks.push_back(task);
    }

    void PortalService::on_file_changed()
    {
        uint32_t new_scheme = get_current_color_scheme();
        if (new_scheme != m_last_color_scheme) {
            m_last_color_scheme = new_scheme;
            
            std::cout << "[PortalService] Color scheme changed to: " << new_scheme << std::endl;
            
            std::vector<std::pair<bool, DbusVariant>> args = {
                {false, std::string("org.freedesktop.appearance")},
                {false, std::string("color-scheme")},
                {true, DbusVariant(new_scheme)}
            };
            
            m_dbus.emit_signal_custom("/org/freedesktop/portal/desktop", 
                               "org.freedesktop.impl.portal.Settings", 
                               "SettingChanged", 
                               args);
        }
    }

    DBusHandlerResult PortalService::handle_message(DBusConnection* conn, DBusMessage* msg)
    {
        if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.Settings", "Read")) {
            handle_read(msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.Settings", "ReadAll")) {
            handle_read_all(conn, msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    void PortalService::handle_read(DBusMessage* msg)
    {
        DBusError error;
        dbus_error_init(&error);

        char* namesp;
        char* key;

        if (!dbus_message_get_args(msg, &error, 
                                   DBUS_TYPE_STRING, &namesp, 
                                   DBUS_TYPE_STRING, &key, 
                                   DBUS_TYPE_INVALID)) {
            m_dbus.send_error(msg, "org.freedesktop.DBus.Error.InvalidArgs", error.message);
            dbus_error_free(&error);
            return;
        }

        std::string ns(namesp);
        std::string k(key);

        std::cout << "[PortalService] Read request - Namespace: " << ns << ", Key: " << k << std::endl;

        if (ns == "org.freedesktop.appearance" && k == "color-scheme") {
            uint32_t scheme = m_last_color_scheme;
            std::cout << "[PortalService] Returning scheme: " << scheme << std::endl;
            std::vector<std::pair<bool, DbusVariant>> args = { {true, DbusVariant(scheme)} };
            // Note: Settings interface returns a variant for the value.
            m_dbus.send_reply_custom(msg, args);
        } else {
            // Unrecognized setting
            std::cout << "[PortalService] Setting not found!" << std::endl;
            m_dbus.send_error(msg, "org.freedesktop.DBus.Error.UnknownMethod", "Setting not found");
        }
    }

    void PortalService::handle_read_all(DBusConnection* conn, DBusMessage* msg)
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (!reply) return;

        DBusMessageIter iter;
        dbus_message_iter_init_append(reply, &iter);

        // a{sa{sv}}
        DBusMessageIter array_iter;
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sa{sv}}", &array_iter);
        
        DBusMessageIter dict_entry_iter;
        dbus_message_iter_open_container(&array_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_entry_iter);
        
        const char* ns = "org.freedesktop.appearance";
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &ns);
        
        // a{sv}
        DBusMessageIter inner_array_iter;
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_ARRAY, "{sv}", &inner_array_iter);
        
        DBusMessageIter inner_dict_entry_iter;
        dbus_message_iter_open_container(&inner_array_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &inner_dict_entry_iter);
        
        const char* key = "color-scheme";
        dbus_message_iter_append_basic(&inner_dict_entry_iter, DBUS_TYPE_STRING, &key);
        
        DBusMessageIter var_iter;
        dbus_message_iter_open_container(&inner_dict_entry_iter, DBUS_TYPE_VARIANT, "u", &var_iter);
        uint32_t scheme = m_last_color_scheme;
        dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT32, &scheme);
        dbus_message_iter_close_container(&inner_dict_entry_iter, &var_iter);
        
        dbus_message_iter_close_container(&inner_array_iter, &inner_dict_entry_iter);
        dbus_message_iter_close_container(&dict_entry_iter, &inner_array_iter);
        dbus_message_iter_close_container(&array_iter, &dict_entry_iter);
        dbus_message_iter_close_container(&iter, &array_iter);
        
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
    }
}
