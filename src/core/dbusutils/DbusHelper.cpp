#include <horizon/dbusutils/DbusHelper.hpp>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace horizon::dbusutils
{
    DbusHelper::DbusHelper(DBusBusType bus_type)
    {
        dbus_threads_init_default();
        DBusError error;
        dbus_error_init(&error);

        m_connection = dbus_bus_get(bus_type, &error);
        if (dbus_error_is_set(&error))
        {
            std::string err_msg = "D-Bus connection error: ";
            err_msg += error.message;
            dbus_error_free(&error);
            throw std::runtime_error(err_msg);
        }

        if (m_connection == nullptr)
        {
            throw std::runtime_error("D-Bus connection is null.");
        }
    }

    DbusHelper::~DbusHelper()
    {
        if (m_connection != nullptr)
        {
            dbus_connection_unref(m_connection);
        }
    }

    DBusMessage* DbusHelper::call_method(const std::string& destination,
                                         const std::string& path,
                                         const std::string& interface,
                                         const std::string& method)
    {
        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr)
        {
            return nullptr;
        }

        DBusError error;
        dbus_error_init(&error);

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, -1, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error))
        {
            dbus_error_free(&error);
            return nullptr;
        }
        return reply;
    }

    void DbusHelper::call_void_method_with_empty_dict(const std::string& destination,
                                                      const std::string& path,
                                                      const std::string& interface,
                                                      const std::string& method)
    {
        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr) return;

        DBusMessageIter iter, dict_iter;
        dbus_message_iter_init_append(msg, &iter);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
        dbus_message_iter_close_container(&iter, &dict_iter);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, -1, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error)) {
            dbus_error_free(&error);
        }
        if (reply) dbus_message_unref(reply);
    }

    DbusVariant DbusHelper::get_property(const std::string& destination,
                                         const std::string& path,
                                         const std::string& interface,
                                         const std::string& property)
    {
        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        "org.freedesktop.DBus.Properties",
                                                        "Get");
        if (msg == nullptr) return "";

        const char* p_interface = interface.c_str();
        const char* p_property = property.c_str();

        dbus_message_append_args(msg,
                                 DBUS_TYPE_STRING, &p_interface,
                                 DBUS_TYPE_STRING, &p_property,
                                 DBUS_TYPE_INVALID);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, -1, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error) || reply == nullptr)
        {
            dbus_error_free(&error);
            return "";
        }

        DBusMessageIter iter;
        dbus_message_iter_init(reply, &iter);

        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
        {
            dbus_message_unref(reply);
            return "";
        }

        DBusMessageIter sub_iter;
        dbus_message_iter_recurse(&iter, &sub_iter);

        int type = dbus_message_iter_get_arg_type(&sub_iter);
        DbusVariant val = "";

        switch (type)
        {
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        {
            const char* s;
            dbus_message_iter_get_basic(&sub_iter, &s);
            val = std::string(s);
            break;
        }
        case DBUS_TYPE_UINT32:
        {
            uint32_t u;
            dbus_message_iter_get_basic(&sub_iter, &u);
            val = u;
            break;
        }
        case DBUS_TYPE_INT32:
        {
            int32_t i;
            dbus_message_iter_get_basic(&sub_iter, &i);
            val = i;
            break;
        }
        case DBUS_TYPE_BOOLEAN:
        {
            dbus_bool_t b;
            dbus_message_iter_get_basic(&sub_iter, &b);
            val = static_cast<bool>(b);
            break;
        }
        case DBUS_TYPE_BYTE:
        {
            uint8_t b;
            dbus_message_iter_get_basic(&sub_iter, &b);
            val = static_cast<uint32_t>(b);
            break;
        }
        case DBUS_TYPE_ARRAY:
        {
            int element_type = dbus_message_iter_get_element_type(&sub_iter);
            if (element_type == DBUS_TYPE_BYTE)
            {
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&sub_iter, &array_iter);
                std::vector<uint8_t> bytes;
                while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
                {
                    uint8_t b;
                    dbus_message_iter_get_basic(&array_iter, &b);
                    bytes.push_back(b);
                    dbus_message_iter_next(&array_iter);
                }
                val = bytes;
            }
            // Add more as needed
            break;
        }
        }

        dbus_message_unref(reply);
        return val;
    }

    std::vector<std::string> DbusHelper::get_all_object_paths(DBusMessage* msg)
    {
        std::vector<std::string> paths;
        if (msg == nullptr) return paths;

        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        while (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INVALID)
        {
            if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH)
            {
                const char* path;
                dbus_message_iter_get_basic(&iter, &path);
                paths.push_back(path);
            }
            dbus_message_iter_next(&iter);
        }

        return paths;
    }

    std::vector<std::string> DbusHelper::get_object_path_list(DBusMessage* msg)
    {
        std::vector<std::string> paths;
        if (msg == nullptr) return paths;

        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        {
            return paths;
        }

        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&iter, &array_iter);

        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_OBJECT_PATH)
        {
            const char* path;
            dbus_message_iter_get_basic(&array_iter, &path);
            paths.push_back(path);
            dbus_message_iter_next(&array_iter);
        }

        return paths;
    }
}
