#pragma once

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <functional>

namespace horizon::dbusutils
{
    /**
     * @brief Represents a D-Bus variant data type.
     */
    using DbusVariant = std::variant<
        std::string,
        uint32_t,
        int32_t,
        uint16_t,
        int16_t,
        uint64_t,
        int64_t,
        double,
        bool,
        std::vector<std::string>,
        std::vector<uint8_t>
    >;

    /**
     * @class DbusHelper
     * @brief A high-level C++ wrapper around the low-level libdbus-1 library.
     */
    class DbusHelper
    {
    public:
        /**
         * @brief Connects to the specified D-Bus bus type.
         * @param bus_type DBUS_BUS_SYSTEM or DBUS_BUS_SESSION.
         */
        DbusHelper(DBusBusType bus_type);
        ~DbusHelper();

        /**
         * @brief Calls a synchronous D-Bus method.
         * @return A DBusMessage* representing the reply. The caller is responsible for unref'ing it.
         */
        DBusMessage* call_method(const std::string& destination,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& method);

        /**
         * @brief Tries to call a method that takes an empty (a{sv}) dict as its only argument.
         */
        void call_void_method_with_empty_dict(const std::string& destination,
                                              const std::string& path,
                                              const std::string& interface,
                                              const std::string& method);

        /**
         * @brief Generic method to get a single property value.
         */
        DbusVariant get_property(const std::string& destination,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& property);

        /**
         * @brief Helper to extract a string list from an array message.
         */
        std::vector<std::string> get_string_list(DBusMessage* msg);

        /**
         * @brief Helper to extract an object path list (as strings) from an array message.
         */
        std::vector<std::string> get_object_path_list(DBusMessage* msg);

        /**
         * @brief Retrieves all object paths provided as top-level arguments in a message.
         */
        std::vector<std::string> get_all_object_paths(DBusMessage* msg);

        /**
         * @brief Returns the underlying D-Bus connection.
         */
        DBusConnection* get_connection() const { return m_connection; }

    private:
        DBusConnection* m_connection{nullptr};
        void check_error(DBusError* error);
    };
}
