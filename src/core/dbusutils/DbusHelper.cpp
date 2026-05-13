#include <horizon/dbusutils/DbusHelper.hpp>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace horizon::dbusutils
{
    static void append_basic_to_iter(DBusMessageIter* iter, const DbusVariant& value);
    static void append_variant_to_iter(DBusMessageIter* parent_iter, const DbusVariant& value);
    static void append_dict(DBusMessageIter* parent_iter, const std::map<std::string, DbusVariant>& options);

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
                                         const std::string& method,
                                         int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

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

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error))
        {
            dbus_error_free(&error);
            return nullptr;
        }
        return reply;
    }

    DBusMessage* DbusHelper::call_method_sync(const std::string& destination,
                                              const std::string& path,
                                              const std::string& interface,
                                              const std::string& method,
                                              const std::vector<DbusVariant>& args,
                                              int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr)
        {
            return nullptr;
        }

        if (!args.empty())
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(msg, &iter);
            for (const auto& arg : args)
            {
                append_basic_to_iter(&iter, arg);
            }
        }

        DBusError error;
        dbus_error_init(&error);

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error))
        {
            std::string err_msg = "D-Bus error in call_method_sync: ";
            err_msg += error.message;
            dbus_error_free(&error);
            if (reply) dbus_message_unref(reply);
            throw std::runtime_error(err_msg);
        }
        return reply;
    }

    void DbusHelper::call_method_void(const std::string& destination,
                                     const std::string& path,
                                     const std::string& interface,
                                     const std::string& method,
                                     int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr) return;

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error)) {
            std::string msg = "D-Bus error in call_method_void: ";
            msg += error.message;
            dbus_error_free(&error);
            if (reply) dbus_message_unref(reply);
            throw std::runtime_error(msg);
        }
        if (reply) dbus_message_unref(reply);
    }

    void DbusHelper::call_void_method_with_empty_dict(const std::string& destination,
                                                      const std::string& path,
                                                      const std::string& interface,
                                                      const std::string& method,
                                                      int timeout_ms)
    {
        call_method_s_asv(destination, path, interface, method, "", {});
    }

    static void append_variant_to_iter(DBusMessageIter* parent_iter, const DbusVariant& value)
    {
        DBusMessageIter var_iter;
        if (std::holds_alternative<std::string>(value))
        {
            const char* s = std::get<std::string>(value).c_str();
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "s", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_STRING, &s);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<bool>(value))
        {
            dbus_bool_t b = std::get<bool>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "b", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_BOOLEAN, &b);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<uint32_t>(value))
        {
            uint32_t u = std::get<uint32_t>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "u", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT32, &u);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<int32_t>(value))
        {
            int32_t i = std::get<int32_t>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "i", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_INT32, &i);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<uint64_t>(value))
        {
            uint64_t u = std::get<uint64_t>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "t", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT64, &u);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<int64_t>(value))
        {
            int64_t i = std::get<int64_t>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "x", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_INT64, &i);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<double>(value))
        {
            double d = std::get<double>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "d", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_DOUBLE, &d);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<std::vector<std::string>>(value))
        {
            const auto& vec = std::get<std::vector<std::string>>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "as", &var_iter);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "s", &array_iter);
            for (const auto& s : vec)
            {
                const char* c_s = s.c_str();
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &c_s);
            }
            dbus_message_iter_close_container(&var_iter, &array_iter);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<std::vector<uint8_t>>(value))
        {
            const auto& vec = std::get<std::vector<uint8_t>>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "ay", &var_iter);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "y", &array_iter);
            for (const auto& b : vec)
            {
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_BYTE, &b);
            }
            dbus_message_iter_close_container(&var_iter, &array_iter);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<ObjectPath>(value))
        {
            const char* s = std::get<ObjectPath>(value).path.c_str();
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "o", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_OBJECT_PATH, &s);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<std::vector<ObjectPath>>(value))
        {
            const auto& vec = std::get<std::vector<ObjectPath>>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "ao", &var_iter);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "o", &array_iter);
            for (const auto& op : vec) {
                const char* s = op.path.c_str();
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_OBJECT_PATH, &s);
            }
            dbus_message_iter_close_container(&var_iter, &array_iter);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
        else if (std::holds_alternative<std::map<std::string, std::string>>(value))
        {
            const auto& map = std::get<std::map<std::string, std::string>>(value);
            dbus_message_iter_open_container(parent_iter, DBUS_TYPE_VARIANT, "a{ss}", &var_iter);
            DBusMessageIter dict_iter;
            dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "{ss}", &dict_iter);
            for (const auto& [k, v] : map) {
                DBusMessageIter entry_iter;
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                const char* pk = k.c_str();
                const char* pv = v.c_str();
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &pk);
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &pv);
                dbus_message_iter_close_container(&dict_iter, &entry_iter);
            }
            dbus_message_iter_close_container(&var_iter, &dict_iter);
            dbus_message_iter_close_container(parent_iter, &var_iter);
        }
    }

    static void append_basic_to_iter(DBusMessageIter* iter, const DbusVariant& value)
    {
        if (std::holds_alternative<std::string>(value))
        {
            const char* s = std::get<std::string>(value).c_str();
            dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &s);
        }
        else if (std::holds_alternative<ObjectPath>(value))
        {
            const char* s = std::get<ObjectPath>(value).path.c_str();
            dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &s);
        }
        else if (std::holds_alternative<std::vector<ObjectPath>>(value))
        {
            const auto& vec = std::get<std::vector<ObjectPath>>(value);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "o", &array_iter);
            for (const auto& op : vec) {
                const char* s = op.path.c_str();
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_OBJECT_PATH, &s);
            }
            dbus_message_iter_close_container(iter, &array_iter);
        }
        else if (std::holds_alternative<std::map<std::string, std::string>>(value))
        {
            const auto& map = std::get<std::map<std::string, std::string>>(value);
            DBusMessageIter dict_iter;
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{ss}", &dict_iter);
            for (const auto& [k, v] : map) {
                DBusMessageIter entry_iter;
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                const char* pk = k.c_str();
                const char* pv = v.c_str();
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &pk);
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &pv);
                dbus_message_iter_close_container(&dict_iter, &entry_iter);
            }
            dbus_message_iter_close_container(iter, &dict_iter);
        }
        else if (std::holds_alternative<bool>(value))
        {
            dbus_bool_t b = std::get<bool>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &b);
        }
        else if (std::holds_alternative<uint32_t>(value))
        {
            uint32_t u = std::get<uint32_t>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &u);
        }
        else if (std::holds_alternative<int32_t>(value))
        {
            int32_t i = std::get<int32_t>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &i);
        }
        else if (std::holds_alternative<uint64_t>(value))
        {
            uint64_t u = std::get<uint64_t>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &u);
        }
        else if (std::holds_alternative<int64_t>(value))
        {
            int64_t i = std::get<int64_t>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &i);
        }
        else if (std::holds_alternative<double>(value))
        {
            double d = std::get<double>(value);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &d);
        }
        else if (std::holds_alternative<std::vector<std::string>>(value))
        {
            const auto& vec = std::get<std::vector<std::string>>(value);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "s", &array_iter);
            for (const auto& s : vec)
            {
                const char* c_s = s.c_str();
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &c_s);
            }
            dbus_message_iter_close_container(iter, &array_iter);
        }
        else if (std::holds_alternative<std::vector<uint8_t>>(value))
        {
            const auto& vec = std::get<std::vector<uint8_t>>(value);
            DBusMessageIter array_iter;
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array_iter);
            for (const auto& b : vec)
            {
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_BYTE, &b);
            }
            dbus_message_iter_close_container(iter, &array_iter);
        }
    }

    static void append_dict(DBusMessageIter* parent_iter, const std::map<std::string, DbusVariant>& options)
    {
        DBusMessageIter dict_iter;
        dbus_message_iter_open_container(parent_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
        
        for (const auto& [key, value] : options)
        {
            DBusMessageIter entry_iter;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            
            const char* c_key = key.c_str();
            dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &c_key);
            
            append_variant_to_iter(&entry_iter, value);
            
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
        }
        
        dbus_message_iter_close_container(parent_iter, &dict_iter);
    }

    void DbusHelper::call_method_s_asv(const std::string& destination,
                                      const std::string& path,
                                      const std::string& interface,
                                      const std::string& method,
                                      const std::string& arg_s,
                                      const std::map<std::string, DbusVariant>& options,
                                      int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr) return;

        DBusMessageIter iter;
        dbus_message_iter_init_append(msg, &iter);
        
        if (!arg_s.empty())
        {
            const char* s = arg_s.c_str();
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &s);
        }
        
        append_dict(&iter, options);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error)) {
            std::string err_msg = "D-Bus error in call_method_s_asv: ";
            err_msg += error.message;
            dbus_error_free(&error);
            if (reply) dbus_message_unref(reply);
            throw std::runtime_error(err_msg);
        }
        if (reply) dbus_message_unref(reply);
    }

    void DbusHelper::call_method_ttss_asv_s_asv(const std::string& destination,
                                              const std::string& path,
                                              const std::string& interface,
                                              const std::string& method,
                                              uint64_t offset, uint64_t size,
                                              const std::string& type, const std::string& name,
                                              const std::map<std::string, DbusVariant>& options,
                                              const std::string& format_type,
                                              const std::map<std::string, DbusVariant>& format_options,
                                              int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr) return;

        DBusMessageIter iter;
        dbus_message_iter_init_append(msg, &iter);
        
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &offset);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &size);
        const char* c_type = type.c_str();
        const char* c_name = name.c_str();
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &c_type);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &c_name);
        
        append_dict(&iter, options);
        
        const char* c_format_type = format_type.c_str();
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &c_format_type);
        
        append_dict(&iter, format_options);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error)) {
            std::string err_msg = "D-Bus error in call_method_ttss_asv_s_asv: ";
            err_msg += error.message;
            dbus_error_free(&error);
            if (reply) dbus_message_unref(reply);
            throw std::runtime_error(err_msg);
        }
        if (reply) dbus_message_unref(reply);
    }

    void DbusHelper::call_method_ttss_asv(const std::string& destination,
                                         const std::string& path,
                                         const std::string& interface,
                                         const std::string& method,
                                         uint64_t offset, uint64_t size,
                                         const std::string& type, const std::string& name,
                                         const std::map<std::string, DbusVariant>& options,
                                         int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        interface.c_str(),
                                                        method.c_str());
        if (msg == nullptr) return;

        DBusMessageIter iter;
        dbus_message_iter_init_append(msg, &iter);
        
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &offset);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &size);
        const char* c_type = type.c_str();
        const char* c_name = name.c_str();
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &c_type);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &c_name);
        
        append_dict(&iter, options);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error)) {
            std::string err_msg = "D-Bus error in call_method_ttss_asv: ";
            err_msg += error.message;
            dbus_error_free(&error);
            if (reply) dbus_message_unref(reply);
            throw std::runtime_error(err_msg);
        }
        if (reply) dbus_message_unref(reply);
    }

    DbusVariant DbusHelper::get_property(const std::string& destination,
                                         const std::string& path,
                                         const std::string& interface,
                                         const std::string& property,
                                         int timeout_ms)
    {
        if (path.empty())
        {
            throw std::runtime_error("D-Bus method call failed: path is empty");
        }

        DBusMessage* msg = dbus_message_new_method_call(destination.c_str(),
                                                        path.c_str(),
                                                        "org.freedesktop.DBus.Properties",
                                                        "Get");
        if (msg == nullptr) return std::string("");

        const char* p_interface = interface.c_str();
        const char* p_property = property.c_str();

        dbus_message_append_args(msg,
                                 DBUS_TYPE_STRING, &p_interface,
                                 DBUS_TYPE_STRING, &p_property,
                                 DBUS_TYPE_INVALID);

        DBusError error;
        dbus_error_init(&error);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_connection, msg, timeout_ms, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error) || reply == nullptr)
        {
            dbus_error_free(&error);
            return std::string("");
        }

        DBusMessageIter iter;
        dbus_message_iter_init(reply, &iter);

        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
        {
            dbus_message_unref(reply);
            return std::string("");
        }

        DBusMessageIter sub_iter;
        dbus_message_iter_recurse(&iter, &sub_iter);

        int type = dbus_message_iter_get_arg_type(&sub_iter);
        DbusVariant val = std::string("");

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
        case DBUS_TYPE_UINT16:
        {
            uint16_t u;
            dbus_message_iter_get_basic(&sub_iter, &u);
            val = static_cast<uint32_t>(u);
            break;
        }
        case DBUS_TYPE_INT16:
        {
            int16_t i;
            dbus_message_iter_get_basic(&sub_iter, &i);
            val = static_cast<int32_t>(i);
            break;
        }
        case DBUS_TYPE_UINT64:
        {
            uint64_t u;
            dbus_message_iter_get_basic(&sub_iter, &u);
            val = u;
            break;
        }
        case DBUS_TYPE_INT64:
        {
            int64_t i;
            dbus_message_iter_get_basic(&sub_iter, &i);
            val = i;
            break;
        }
        case DBUS_TYPE_DOUBLE:
        {
            double d;
            dbus_message_iter_get_basic(&sub_iter, &d);
            val = d;
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
            else if (element_type == DBUS_TYPE_STRING || element_type == DBUS_TYPE_OBJECT_PATH)
            {
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&sub_iter, &array_iter);
                std::vector<std::string> strings;
                while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
                {
                    const char* s;
                    dbus_message_iter_get_basic(&array_iter, &s);
                    strings.push_back(std::string(s));
                    dbus_message_iter_next(&array_iter);
                }
                val = strings;
            }
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
    void DbusHelper::add_match_rule(const std::string& rule)
    {
        DBusError error;
        dbus_error_init(&error);
        dbus_bus_add_match(m_connection, rule.c_str(), &error);
        dbus_connection_flush(m_connection);
        if (dbus_error_is_set(&error))
        {
            std::string msg = "D-Bus add_match error: ";
            msg += error.message;
            dbus_error_free(&error);
            // We might want to log this instead of throwing, but for now:
            std::cerr << msg << std::endl;
        }
    }

    void DbusHelper::remove_match_rule(const std::string& rule)
    {
        DBusError error;
        dbus_error_init(&error);
        dbus_bus_remove_match(m_connection, rule.c_str(), &error);
        dbus_connection_flush(m_connection);
        if (dbus_error_is_set(&error))
        {
            dbus_error_free(&error);
        }
    }

    DBusMessage* DbusHelper::pop_message(int timeout_ms)
    {
        if (m_connection == nullptr) return nullptr;

        dbus_connection_read_write(m_connection, timeout_ms);
        return dbus_connection_pop_message(m_connection);
    }

    bool DbusHelper::process_events(int timeout_ms)
    {
        if (m_connection == nullptr) return false;
        return dbus_connection_read_write_dispatch(m_connection, timeout_ms);
    }

    bool DbusHelper::request_name(const std::string& name)
    {
        DBusError error;
        dbus_error_init(&error);
        int result = dbus_bus_request_name(m_connection, name.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
        if (dbus_error_is_set(&error))
        {
            printf("[D-Bus] Error requesting name %s: %s\n", name.c_str(), error.message);
            dbus_error_free(&error);
            return false;
        }
        
        if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) return true;
        if (result == DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER) return true;
        
        printf("[D-Bus] Could not acquire name %s (Result code: %d)\n", name.c_str(), result);
        return false;
    }

    static DBusHandlerResult object_message_handler(DBusConnection* conn, DBusMessage* msg, void* user_data)
    {
        printf("[D-Bus Core] Message received for object at path: %s\n", dbus_message_get_path(msg));
        fflush(stdout);
        auto* object = static_cast<DbusObject*>(user_data);
        return object->handle_message(conn, msg);
    }

    void DbusHelper::register_object(const std::string& path, DbusObject* object)
    {
        DBusObjectPathVTable vtable = { nullptr, object_message_handler, nullptr, nullptr, nullptr, nullptr };
        dbus_connection_register_object_path(m_connection, path.c_str(), &vtable, object);
    }

    void DbusHelper::register_fallback(const std::string& path, DbusObject* object)
    {
        DBusObjectPathVTable vtable = { nullptr, object_message_handler, nullptr, nullptr, nullptr, nullptr };
        dbus_connection_register_fallback(m_connection, path.c_str(), &vtable, object);
    }

    void DbusHelper::unregister_object(const std::string& path)
    {
        dbus_connection_unregister_object_path(m_connection, path.c_str());
    }

    void DbusHelper::send_reply(DBusMessage* msg, const std::vector<DbusVariant>& args)
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (!reply) return;

        if (!args.empty())
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(reply, &iter);
            for (const auto& arg : args)
            {
                append_basic_to_iter(&iter, arg);
            }
        }

        dbus_connection_send(m_connection, reply, nullptr);
        dbus_message_unref(reply);
    }

    void DbusHelper::send_reply_custom(DBusMessage* msg, const std::vector<std::pair<bool, DbusVariant>>& args)
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (!reply) return;

        DBusMessageIter iter;
        dbus_message_iter_init_append(reply, &iter);

        for (const auto& [as_variant, value] : args)
        {
            if (as_variant) {
                append_variant_to_iter(&iter, value);
            } else {
                append_basic_to_iter(&iter, value);
            }
        }

        dbus_connection_send(m_connection, reply, nullptr);
        dbus_message_unref(reply);
    }

    void DbusHelper::send_error(DBusMessage* msg, const std::string& error_name, const std::string& error_message)
    {
        DBusMessage* error = dbus_message_new_error(msg, error_name.c_str(), error_message.c_str());
        if (!error) return;
        dbus_connection_send(m_connection, error, nullptr);
        dbus_message_unref(error);
    }

    void DbusHelper::emit_signal(const std::string& path, const std::string& interface, const std::string& signal, const std::vector<DbusVariant>& args)
    {
        DBusMessage* sig = dbus_message_new_signal(path.c_str(), interface.c_str(), signal.c_str());
        if (!sig) return;

        if (!args.empty())
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(sig, &iter);
            for (const auto& arg : args)
            {
                append_basic_to_iter(&iter, arg);
            }
        }

        dbus_connection_send(m_connection, sig, nullptr);
        dbus_message_unref(sig);
    }
}
