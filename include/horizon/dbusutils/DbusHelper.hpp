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
     * @class DbusObject
     * @brief Base class for objects exported over D-Bus.
     */
    class DbusObject
    {
    public:
        virtual ~DbusObject() = default;
        virtual DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) = 0;
    };

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
                                 const std::string& method,
                                 int timeout_ms = -1);
        
        /**
         * @brief Calls a D-Bus method that takes no arguments and returns nothing (or result is ignored).
         */
        void call_method_void(const std::string& destination,
                             const std::string& path,
                             const std::string& interface,
                             const std::string& method,
                             int timeout_ms = -1);

        /**
         * @brief Tries to call a method that takes an empty (a{sv}) dict as its only argument.
         */
        void call_void_method_with_empty_dict(const std::string& destination,
                                              const std::string& path,
                                              const std::string& interface,
                                              const std::string& method,
                                              int timeout_ms = -1);

        /**
         * @brief Calls a method with (s, a{sv}) signature. Used for UDisks2 Block.Format.
         */
        void call_method_s_asv(const std::string& destination,
                               const std::string& path,
                               const std::string& interface,
                               const std::string& method,
                               const std::string& arg_s,
                               const std::map<std::string, DbusVariant>& options,
                               int timeout_ms = -1);

        /**
         * @brief Calls a method with (ttss, a{sv}, s, a{sv}) signature. Used for UDisks2 PartitionTable.CreatePartitionAndFormat.
         */
        void call_method_ttss_asv_s_asv(const std::string& destination,
                                       const std::string& path,
                                       const std::string& interface,
                                       const std::string& method,
                                       uint64_t offset, uint64_t size,
                                       const std::string& type, const std::string& name,
                                       const std::map<std::string, DbusVariant>& options,
                                       const std::string& format_type,
                                       const std::map<std::string, DbusVariant>& format_options,
                                       int timeout_ms = -1);

        /**
         * @brief Calls a method with (ttss, a{sv}) signature. Used for UDisks2 PartitionTable.CreatePartition.
         */
        void call_method_ttss_asv(const std::string& destination,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& method,
                                 uint64_t offset, uint64_t size,
                                 const std::string& type, const std::string& name,
                                 const std::map<std::string, DbusVariant>& options,
                                 int timeout_ms = -1);

        /**
         * @brief Generic method to get a single property value.
         */
        DbusVariant get_property(const std::string& destination,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& property,
                                 int timeout_ms = -1);

        /**
         * @brief Helper to extract a string list from an array message.
         */
        std::vector<std::string> get_string_list(DBusMessage* msg);

        /**
         * @brief Helper to extract an object path list (as strings) from an array message.
         */
        std::vector<std::string> get_object_path_list(DBusMessage* msg);

        /**
         * @brief Adds a match rule to listen for specific signals.
         * @param rule The D-Bus match rule (e.g., "type='signal',interface='org.freedesktop.DBus.Properties'").
         */
        void add_match_rule(const std::string& rule);

        /**
         * @brief Removes a match rule.
         */
        void remove_match_rule(const std::string& rule);

        /**
         * @brief Checks for incoming messages and returns the next one if available.
         * @param timeout_ms Maximum time to wait for a message.
         * @return A DBusMessage* representing the message, or nullptr if none. The caller is responsible for unref'ing it.
         */
        DBusMessage* pop_message(int timeout_ms = 0);

        /**
         * @brief Reads, writes and dispatches messages.
         * @param timeout_ms Maximum time to wait.
         * @return True if the connection is still open.
         */
        bool process_events(int timeout_ms = 100);

        /**
         * @brief Retrieves all object paths provided as top-level arguments in a message.
         */
        std::vector<std::string> get_all_object_paths(DBusMessage* msg);

        /**
         * @brief Returns the underlying D-Bus connection.
         */
        DBusConnection* get_connection() const { return m_connection; }

        /**
         * @brief Requests a name on the D-Bus bus.
         * @param name The name to request (e.g., "org.freedesktop.Secrets").
         * @return True if successful.
         */
        bool request_name(const std::string& name);

        /**
         * @brief Registers an object path on the D-Bus bus.
         * @param path The object path (e.g., "/org/freedesktop/secrets").
         * @param object The object instance to handle messages for this path.
         */
        void register_object(const std::string& path, DbusObject* object);

        /**
         * @brief Unregisters an object path.
         */
        void unregister_object(const std::string& path);

        /**
         * @brief Sends a method return reply to a pending message.
         */
        void send_reply(DBusMessage* msg, const std::vector<DbusVariant>& args = {});

        /**
         * @brief Sends an error reply to a pending message.
         */
        void send_error(DBusMessage* msg, const std::string& error_name, const std::string& error_message);

        /**
         * @brief Emits a signal from the specified path.
         */
        void emit_signal(const std::string& path, const std::string& interface, const std::string& signal, const std::vector<DbusVariant>& args = {});

    private:
        DBusConnection* m_connection{nullptr};
        void check_error(DBusError* error);
    };
}
