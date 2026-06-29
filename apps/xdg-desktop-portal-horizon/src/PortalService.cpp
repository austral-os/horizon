#include "PortalService.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <horizon/dialogs/FileDialog.hpp>

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
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.FileChooser", "OpenFile")) {
            handle_file_chooser(conn, msg, 0); // 0 = Open
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.FileChooser", "SaveFile")) {
            handle_file_chooser(conn, msg, 1); // 1 = Save
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.FileChooser", "SaveFiles")) {
            handle_file_chooser(conn, msg, 2); // 2 = SaveFiles (unsupported by horizon, fallback to Save)
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "Get")) {
            DBusError error;
            dbus_error_init(&error);
            char* interface_name;
            char* property_name;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &interface_name, DBUS_TYPE_STRING, &property_name, DBUS_TYPE_INVALID)) {
                if (std::string(property_name) == "version") {
                    uint32_t version = 1;
                    DBusMessage* reply = dbus_message_new_method_return(msg);
                    if (reply) {
                        DBusMessageIter r_iter, var_iter;
                        dbus_message_iter_init_append(reply, &r_iter);
                        dbus_message_iter_open_container(&r_iter, DBUS_TYPE_VARIANT, "u", &var_iter);
                        dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT32, &version);
                        dbus_message_iter_close_container(&r_iter, &var_iter);
                        dbus_connection_send(conn, reply, nullptr);
                        dbus_message_unref(reply);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            }
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "GetAll")) {
            uint32_t version = 1;
            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply) {
                DBusMessageIter r_iter, array_iter, dict_iter, var_iter;
                dbus_message_iter_init_append(reply, &r_iter);
                dbus_message_iter_open_container(&r_iter, DBUS_TYPE_ARRAY, "{sv}", &array_iter);
                
                dbus_message_iter_open_container(&array_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_iter);
                const char* key = "version";
                dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key);
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "u", &var_iter);
                dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT32, &version);
                dbus_message_iter_close_container(&dict_iter, &var_iter);
                dbus_message_iter_close_container(&array_iter, &dict_iter);
                
                dbus_message_iter_close_container(&r_iter, &array_iter);
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect")) {
            std::string xml = 
                "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
                "<node>\n"
                "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                "    <method name=\"Introspect\">\n"
                "      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
                "    <method name=\"Get\">\n"
                "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAll\">\n"
                "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"properties\" type=\"a{sv}\" direction=\"out\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "  <interface name=\"org.freedesktop.impl.portal.Settings\">\n"
                "    <property name=\"version\" type=\"u\" access=\"read\"/>\n"
                "    <method name=\"Read\">\n"
                "      <arg name=\"namespace\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"key\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
                "    </method>\n"
                "    <method name=\"ReadAll\">\n"
                "      <arg name=\"namespaces\" type=\"as\" direction=\"in\"/>\n"
                "      <arg name=\"value\" type=\"a{sa{sv}}\" direction=\"out\"/>\n"
                "    </method>\n"
                "    <signal name=\"SettingChanged\">\n"
                "      <arg name=\"namespace\" type=\"s\"/>\n"
                "      <arg name=\"key\" type=\"s\"/>\n"
                "      <arg name=\"value\" type=\"v\"/>\n"
                "    </signal>\n"
                "  </interface>\n"
                "  <interface name=\"org.freedesktop.impl.portal.FileChooser\">\n"
                "    <property name=\"version\" type=\"u\" access=\"read\"/>\n"
                "    <method name=\"OpenFile\">\n"
                "      <arg name=\"handle\" type=\"o\" direction=\"in\"/>\n"
                "      <arg name=\"app_id\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"parent_window\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"title\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"options\" type=\"a{sv}\" direction=\"in\"/>\n"
                "      <arg name=\"response\" type=\"u\" direction=\"out\"/>\n"
                "      <arg name=\"results\" type=\"a{sv}\" direction=\"out\"/>\n"
                "    </method>\n"
                "    <method name=\"SaveFile\">\n"
                "      <arg name=\"handle\" type=\"o\" direction=\"in\"/>\n"
                "      <arg name=\"app_id\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"parent_window\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"title\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"options\" type=\"a{sv}\" direction=\"in\"/>\n"
                "      <arg name=\"response\" type=\"u\" direction=\"out\"/>\n"
                "      <arg name=\"results\" type=\"a{sv}\" direction=\"out\"/>\n"
                "    </method>\n"
                "    <method name=\"SaveFiles\">\n"
                "      <arg name=\"handle\" type=\"o\" direction=\"in\"/>\n"
                "      <arg name=\"app_id\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"parent_window\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"title\" type=\"s\" direction=\"in\"/>\n"
                "      <arg name=\"options\" type=\"a{sv}\" direction=\"in\"/>\n"
                "      <arg name=\"response\" type=\"u\" direction=\"out\"/>\n"
                "      <arg name=\"results\" type=\"a{sv}\" direction=\"out\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "</node>\n";
            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply) {
                DBusMessageIter r_iter;
                dbus_message_iter_init_append(reply, &r_iter);
                const char* xml_c = xml.c_str();
                dbus_message_iter_append_basic(&r_iter, DBUS_TYPE_STRING, &xml_c);
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
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

    void PortalService::handle_file_chooser(DBusConnection* conn, DBusMessage* msg, int mode)
    {
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        const char *handle = nullptr, *app_id = nullptr, *parent_window = nullptr, *title = nullptr;
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH) {
            dbus_message_iter_get_basic(&iter, &handle);
            dbus_message_iter_next(&iter);
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &app_id);
            dbus_message_iter_next(&iter);
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &parent_window);
            dbus_message_iter_next(&iter);
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &title);
            dbus_message_iter_next(&iter);
        }

        bool directory = false;
        bool multiple = false;
        std::vector<horizon::FileFilter> file_filters;

        // Parse options (a{sv})
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
            DBusMessageIter dict_iter;
            dbus_message_iter_recurse(&iter, &dict_iter);
            while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter entry_iter;
                dbus_message_iter_recurse(&dict_iter, &entry_iter);
                const char* key;
                dbus_message_iter_get_basic(&entry_iter, &key);
                dbus_message_iter_next(&entry_iter);

                if (std::string(key) == "directory") {
                    DBusMessageIter var_iter;
                    dbus_message_iter_recurse(&entry_iter, &var_iter);
                    dbus_bool_t val;
                    if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_BOOLEAN) {
                        dbus_message_iter_get_basic(&var_iter, &val);
                        directory = val;
                    }
                } else if (std::string(key) == "multiple") {
                    DBusMessageIter var_iter;
                    dbus_message_iter_recurse(&entry_iter, &var_iter);
                    dbus_bool_t val;
                    if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_BOOLEAN) {
                        dbus_message_iter_get_basic(&var_iter, &val);
                        multiple = val;
                    }
                } else if (std::string(key) == "filters") {
                    DBusMessageIter var_iter;
                    dbus_message_iter_recurse(&entry_iter, &var_iter);
                    if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_ARRAY) {
                        DBusMessageIter array_iter;
                        dbus_message_iter_recurse(&var_iter, &array_iter);
                        
                        std::vector<std::string> all_patterns;

                        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
                            DBusMessageIter struct_iter;
                            dbus_message_iter_recurse(&array_iter, &struct_iter);
                            
                            const char* filter_name = nullptr;
                            if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_STRING) {
                                dbus_message_iter_get_basic(&struct_iter, &filter_name);
                            }
                            dbus_message_iter_next(&struct_iter);
                            
                            horizon::FileFilter filter;
                            if (filter_name) filter.name = filter_name;
                            
                            if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_ARRAY) {
                                DBusMessageIter filter_array_iter;
                                dbus_message_iter_recurse(&struct_iter, &filter_array_iter);
                                while (dbus_message_iter_get_arg_type(&filter_array_iter) == DBUS_TYPE_STRUCT) {
                                    DBusMessageIter filter_item_iter;
                                    dbus_message_iter_recurse(&filter_array_iter, &filter_item_iter);
                                    
                                    uint32_t type = 0;
                                    if (dbus_message_iter_get_arg_type(&filter_item_iter) == DBUS_TYPE_UINT32) {
                                        dbus_message_iter_get_basic(&filter_item_iter, &type);
                                    }
                                    dbus_message_iter_next(&filter_item_iter);
                                    
                                    const char* pattern = nullptr;
                                    if (dbus_message_iter_get_arg_type(&filter_item_iter) == DBUS_TYPE_STRING) {
                                        dbus_message_iter_get_basic(&filter_item_iter, &pattern);
                                    }
                                    
                                    if (pattern) {
                                        // For mime types (type=1), map roughly to extension.
                                        // In a real scenario we'd query a mime-db. Here we just take the sub-type if it's not glob.
                                        std::string pat_str = pattern;
                                        if (type == 1 && pat_str.find('/') != std::string::npos) {
                                            pat_str = "*." + pat_str.substr(pat_str.find('/') + 1);
                                        }
                                        filter.patterns.push_back(pat_str);
                                        all_patterns.push_back(pat_str);
                                    }
                                    dbus_message_iter_next(&filter_array_iter);
                                }
                            }
                            file_filters.push_back(filter);
                            dbus_message_iter_next(&array_iter);
                        }
                        
                        if (file_filters.size() > 1) {
                            horizon::FileFilter all_filter;
                            all_filter.name = "All Supported Files";
                            all_filter.patterns = all_patterns;
                            file_filters.insert(file_filters.begin(), all_filter);
                        }
                    }
                }
                dbus_message_iter_next(&dict_iter);
            }
        }

        std::string handle_path = handle ? handle : "";
        std::string s_title = title ? title : "";

        // Register the Request object
        auto request_obj = new RequestObject(m_dbus, handle_path);
        if (!handle_path.empty()) {
            m_dbus.register_object(handle_path, request_obj);
        }

        dbus_message_ref(msg);

        // Run dialog in a new thread
        std::thread([this, conn, msg, request_obj, mode, directory, multiple, s_title, handle_path, file_filters]() {
            FileDialogMode dialog_mode = FileDialogMode::Open;
            if (mode == 1 || mode == 2) dialog_mode = FileDialogMode::Save;
            if (directory) dialog_mode = FileDialogMode::SelectFolder;

            auto dialog = std::make_unique<horizon::FileDialog>(dialog_mode, s_title);
            if (mode == 0 && !directory) {
                dialog->set_select_multiple(multiple);
            }
            
            if (!file_filters.empty()) {
                dialog->set_filters(file_filters);
            }
            
            uint32_t response_code = 1; // Default to cancel
            std::vector<std::string> uris;

            bool done = false;

            dialog->when_accepted.connect([&](horizon::FileDialogAcceptedContext& ctx) {
                response_code = 0; // Success
                if (!ctx.selected_paths.empty()) {
                    for (const auto& path : ctx.selected_paths) {
                        uris.push_back("file://" + path);
                    }
                } else if (!ctx.selected_path.empty()) {
                    uris.push_back("file://" + ctx.selected_path);
                }
                done = true;
                dialog->quit();
            });

            dialog->when_cancelled.connect([&](horizon::FileDialogCancelledContext&) {
                response_code = 1; // Cancel
                done = true;
                dialog->quit();
            });

            // Initialize and block
            dialog->initialize();
            dialog->run();

            // After dialog finishes, if request was closed by portal, we might not need to reply,
            // but we'll reply anyway or portal ignores it.
            if (request_obj->is_closed()) {
                response_code = 2; // Aborted
            }

            // Create DBus reply: (u response, a{sv} results)
            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply) {
                DBusMessageIter r_iter;
                dbus_message_iter_init_append(reply, &r_iter);
                dbus_message_iter_append_basic(&r_iter, DBUS_TYPE_UINT32, &response_code);
                
                // a{sv}
                DBusMessageIter array_iter;
                dbus_message_iter_open_container(&r_iter, DBUS_TYPE_ARRAY, "{sv}", &array_iter);
                
                if (response_code == 0 && !uris.empty()) {
                    DBusMessageIter dict_entry_iter;
                    dbus_message_iter_open_container(&array_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_entry_iter);
                    
                    const char* key = "uris";
                    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
                    
                    DBusMessageIter var_iter;
                    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "as", &var_iter);
                    
                    DBusMessageIter as_iter;
                    dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "s", &as_iter);
                    for (const auto& uri : uris) {
                        const char* uri_c = uri.c_str();
                        dbus_message_iter_append_basic(&as_iter, DBUS_TYPE_STRING, &uri_c);
                    }
                    dbus_message_iter_close_container(&var_iter, &as_iter);
                    dbus_message_iter_close_container(&dict_entry_iter, &var_iter);
                    dbus_message_iter_close_container(&array_iter, &dict_entry_iter);
                }
                
                dbus_message_iter_close_container(&r_iter, &array_iter);
                
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }

            if (!handle_path.empty()) {
                m_dbus.unregister_object(handle_path);
            }
            delete request_obj;

            dbus_message_unref(msg);
        }).detach();
    }

    DBusHandlerResult RequestObject::handle_message(DBusConnection* conn, DBusMessage* msg)
    {
        if (dbus_message_is_method_call(msg, "org.freedesktop.impl.portal.Request", "Close")) {
            m_closed = true;
            // The dialog thread will check this, but ideally we should signal it to close.
            // For simplicity, we just mark it.

            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply) {
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect")) {
            std::string xml = 
                "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
                "<node>\n"
                "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                "    <method name=\"Introspect\">\n"
                "      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "  <interface name=\"org.freedesktop.impl.portal.Request\">\n"
                "    <method name=\"Close\">\n"
                "    </method>\n"
                "  </interface>\n"
                "</node>\n";
            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply) {
                DBusMessageIter r_iter;
                dbus_message_iter_init_append(reply, &r_iter);
                const char* xml_c = xml.c_str();
                dbus_message_iter_append_basic(&r_iter, DBUS_TYPE_STRING, &xml_c);
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}
