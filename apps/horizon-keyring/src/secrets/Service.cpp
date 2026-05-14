#include "Service.hpp"
#include <horizon/Logger.hpp>
#include <iostream>
#include <map>
#include <algorithm>
#include <cstring>
#include <dbus/dbus.h>
#include <unistd.h>
#include <openssl/rand.h>
#include <fstream>

namespace horizon::secrets
{
    Service::Service(dbusutils::DbusHelper& dbus) : m_dbus(dbus)
    {
        m_crypto = std::make_unique<crypto::CryptoManager>();
        init_storage();
        init_pam_listener();
        check_pending_unlock();
    }

    void Service::init_storage()
    {
        std::string home = getenv("HOME");
        std::string db_dir = home + "/.local/share/horizon";
        system(("mkdir -p " + db_dir).c_str());
        
        m_storage = std::make_unique<storage::StorageManager>(db_dir + "/keyring.db");
        m_storage->init_database();
        m_storage->create_collection("Default", "default");
        std::cout << "[Horizon Keyring] Storage initialized at " << db_dir << "/keyring.db" << std::endl;
    }

    void Service::init_pam_listener()
    {
        m_socket_listener = std::make_unique<SocketListener>(getuid(), [this](const std::string& password) {
            unlock_keyring(password);
        });
        m_socket_listener->start();
        std::cout << "[Horizon Keyring] PAM listener started" << std::endl;
    }

    void Service::unlock_keyring(const std::string& password)
    {
        LOG_INFO << "[Horizon Keyring] Received password from PAM, deriving master key...";
        
        std::vector<uint8_t> salt = m_storage->get_master_salt();
        
        try {
            m_master_key = m_crypto->derive_key(password, salt);
            
            // Now handle the DB Key (Professional Key Wrapping)
            std::vector<uint8_t> encrypted_db_key = m_storage->get_encrypted_db_key();
            if (encrypted_db_key.empty()) {
                LOG_INFO << "[Horizon Keyring] First run: Generating new Database Key...";
                m_db_key.resize(32);
                RAND_bytes(m_db_key.data(), m_db_key.size());
                
                // Encrypt it with Master Key and save
                std::vector<uint8_t> encrypted = m_crypto->encrypt(m_db_key, m_master_key);
                m_storage->set_encrypted_db_key(encrypted);
            } else {
                // Decrypt existing DB key using Master Key
                m_db_key = m_crypto->decrypt(encrypted_db_key, m_master_key);
            }
            
            LOG_INFO << "[Horizon Keyring] Keyring fully unlocked (Master Key + DB Key).";
        } catch (const std::exception& e) {
            LOG_ERROR << "[Horizon Keyring] Failed to unlock keyring: " << e.what();
            m_master_key.clear();
            m_db_key.clear();
        }
    }

    DBusHandlerResult Service::handle_message(DBusConnection* conn, DBusMessage* msg)
    {
        std::string interface = dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "";
        std::string method = dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "";
        std::string path = dbus_message_get_path(msg) ? dbus_message_get_path(msg) : "";

        // Log incoming call (disabled in production to keep logs clean)
        // LOG_DEBUG << "[Horizon Keyring] Incoming D-Bus call: " << interface << "." << method << " at " << path;

        if (interface == "org.freedesktop.Secret.Service") {
            if (method == "OpenSession") {
                LOG_INFO << "[Horizon Keyring] Method call: OpenSession";
                handle_open_session(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "CreateCollection") {
                handle_create_collection(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "SearchItems") {
                handle_search_items(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "GetSecrets") {
                handle_get_secrets(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "ReadAlias") {
                handle_read_alias(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "Unlock") {
                handle_unlock(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.freedesktop.Secret.Collection") {
            if (method == "CreateItem") {
                handle_create_item(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.freedesktop.Secret.Item") {
            if (method == "Delete") {
                handle_delete_item(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.gnome.keyring.Daemon") {
            if (method == "GetControlDirectory") {
                LOG_INFO << "[Horizon Keyring] Handling GNOME GetControlDirectory stub";
                m_dbus.send_reply(msg, {std::string("/tmp/horizon-keyring-stub")});
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.freedesktop.DBus.ObjectManager") {
            if (method == "GetManagedObjects") {
                LOG_INFO << "[Horizon Keyring] Handling ObjectManager.GetManagedObjects";
                m_dbus.send_reply(msg, {});
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.freedesktop.DBus.Properties") {
            if (method == "Get") {
                handle_get_property(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "GetAll") {
                handle_get_all_properties(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "Set") {
                handle_set_property(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        } else if (interface == "org.freedesktop.DBus.Introspectable" && method == "Introspect") {
            std::string xml;
            if (path.find("/collection/default/") != std::string::npos) {
                // Introspection for an ITEM
                xml = R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.Secret.Item">
    <method name="Delete">
      <arg name="prompt" type="o" direction="out"/>
    </method>
    <property name="Label" type="s" access="readwrite"/>
    <property name="Attributes" type="a{ss}" access="readwrite"/>
  </interface>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg name="interface" type="s" direction="in"/>
      <arg name="propname" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="GetAll">
      <arg name="interface" type="s" direction="in"/>
      <arg name="props" type="a{sv}" direction="out"/>
    </method>
    <method name="Set">
      <arg name="interface" type="s" direction="in"/>
      <arg name="propname" type="s" direction="in"/>
      <arg name="value" type="v" direction="in"/>
    </method>
  </interface>
</node>)";
            } else {
                // Introspection for the SERVICE
                xml = R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.Secret.Service">
    <method name="OpenSession">
      <arg name="algorithm" type="s" direction="in"/>
      <arg name="input" type="v" direction="in"/>
      <arg name="output" type="v" direction="out"/>
      <arg name="result" type="o" direction="out"/>
    </method>
    <method name="CreateCollection">
      <arg name="properties" type="a{sv}" direction="in"/>
      <arg name="alias" type="s" direction="in"/>
      <arg name="collection" type="o" direction="out"/>
      <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="SearchItems">
      <arg name="attributes" type="a{sv}" direction="in"/>
      <arg name="unlocked" type="ao" direction="out"/>
      <arg name="locked" type="ao" direction="out"/>
    </method>
    <method name="ReadAlias">
      <arg name="name" type="s" direction="in"/>
      <arg name="collection" type="o" direction="out"/>
    </method>
    <method name="Unlock">
      <arg name="objects" type="ao" direction="in"/>
      <arg name="unlocked" type="ao" direction="out"/>
      <arg name="prompt" type="o" direction="out"/>
    </method>
  </interface>
</node>)";
            }
            m_dbus.send_reply(msg, {xml});
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    void Service::handle_open_session(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: OpenSession";
        m_dbus.send_reply_custom(msg, {
            {true, std::string("")}, // output variant (true = as variant)
            {false, dbusutils::ObjectPath("/org/freedesktop/secrets/session/plain")} // session path
        });
    }

    void Service::handle_create_collection(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: CreateCollection";
        m_dbus.send_reply(msg, {
            dbusutils::ObjectPath("/org/freedesktop/secrets/collection/default"), // collection path
            dbusutils::ObjectPath("/")  // prompt path stub (none)
        });
    }

    void Service::handle_read_alias(DBusMessage* msg)
    {
        const char* name;
        dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);
        
        LOG_INFO << "[Horizon Keyring] ReadAlias called for: " << (name ? name : "null");

        if (name && std::string(name) == "default") {
            // Return our Default collection path
            m_dbus.send_reply(msg, {dbusutils::ObjectPath("/org/freedesktop/secrets/collection/default")});
        } else {
            m_dbus.send_reply(msg, {dbusutils::ObjectPath("/")}); // Not found
        }
    }

    void Service::handle_unlock(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: Unlock";
        
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        
        std::vector<dbusutils::ObjectPath> unlocked_paths;
        
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
            DBusMessageIter array_iter;
            dbus_message_iter_recurse(&iter, &array_iter);
            while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_OBJECT_PATH) {
                const char* path;
                dbus_message_iter_get_basic(&array_iter, &path);
                unlocked_paths.push_back(dbusutils::ObjectPath(path));
                dbus_message_iter_next(&array_iter);
            }
        }

        // If the master key is already present, it means the keyring is unlocked (via PAM).
        // We return all requested paths as "unlocked" and no prompt.
        if (!m_master_key.empty()) {
            LOG_INFO << "[Horizon Keyring] Keyring already unlocked, returning success for all paths.";
            m_dbus.send_reply(msg, {
                dbusutils::DbusVariant(unlocked_paths),
                dbusutils::ObjectPath("/") // No prompt needed
            });
        } else {
            // In a real implementation, if locked, we should return a Prompt object.
            // For now, we return empty unlocked list and no prompt (which might cause client failure, 
            // but at least the method exists).
            LOG_WARNING << "[Horizon Keyring] Unlock called but keyring is LOCKED (no PAM password yet).";
            m_dbus.send_reply(msg, {
                dbusutils::DbusVariant(std::vector<dbusutils::ObjectPath>{}),
                dbusutils::ObjectPath("/") 
            });
        }
    }
    
    void Service::handle_create_item(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: CreateItem (Full implementation)";
        
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        // 1. Properties (a{sv})
        std::string label = "Untitled";
        std::map<std::string, std::string> attributes;
        
        DBusMessageIter dict_iter;
        dbus_message_iter_recurse(&iter, &dict_iter);
        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter;
            dbus_message_iter_recurse(&dict_iter, &entry_iter);
            
            const char* key;
            dbus_message_iter_get_basic(&entry_iter, &key);
            dbus_message_iter_next(&entry_iter);
            
            DBusMessageIter var_iter;
            dbus_message_iter_recurse(&entry_iter, &var_iter);
            
            if (std::string(key) == "org.freedesktop.Secret.Item.Label") {
                const char* val;
                dbus_message_iter_get_basic(&var_iter, &val);
                label = val;
            } else if (std::string(key) == "org.freedesktop.Secret.Item.Attributes") {
                DBusMessageIter attr_dict_iter;
                dbus_message_iter_recurse(&var_iter, &attr_dict_iter);
                while (dbus_message_iter_get_arg_type(&attr_dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                    DBusMessageIter attr_entry_iter;
                    dbus_message_iter_recurse(&attr_dict_iter, &attr_entry_iter);
                    const char* a_key;
                    const char* a_val;
                    dbus_message_iter_get_basic(&attr_entry_iter, &a_key);
                    dbus_message_iter_next(&attr_entry_iter);
                    dbus_message_iter_get_basic(&attr_entry_iter, &a_val);
                    attributes[a_key] = a_val;
                    dbus_message_iter_next(&attr_dict_iter);
                }
            }
            dbus_message_iter_next(&dict_iter);
        }
        dbus_message_iter_next(&iter);

        // 2. Secret (oayays)
        std::vector<uint8_t> secret_data;
        DBusMessageIter struct_iter;
        dbus_message_iter_recurse(&iter, &struct_iter);
        
        // Skip session path (o)
        dbus_message_iter_next(&struct_iter);
        
        // Parameters (ay)
        DBusMessageIter param_iter;
        dbus_message_iter_recurse(&struct_iter, &param_iter);
        std::vector<uint8_t> param_data;
        while (dbus_message_iter_get_arg_type(&param_iter) == DBUS_TYPE_BYTE) {
            uint8_t b;
            dbus_message_iter_get_basic(&param_iter, &b);
            param_data.push_back(b);
            dbus_message_iter_next(&param_iter);
        }
        dbus_message_iter_next(&struct_iter);
        
        // Secret value (ay)
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&struct_iter, &array_iter);
        std::vector<uint8_t> value_data;
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_BYTE) {
            uint8_t b;
            dbus_message_iter_get_basic(&array_iter, &b);
            value_data.push_back(b);
            dbus_message_iter_next(&array_iter);
        }

        // Bundle them: [param_len (1 byte)] + [param_data] + [value_data]
        std::vector<uint8_t> secret_bundle;
        secret_bundle.push_back((uint8_t)param_data.size());
        secret_bundle.insert(secret_bundle.end(), param_data.begin(), param_data.end());
        secret_bundle.insert(secret_bundle.end(), value_data.begin(), value_data.end());

        // Encrypt bundle using the Database Key
        std::vector<uint8_t> final_data;
        if (!m_db_key.empty()) {
            try {
                final_data = m_crypto->encrypt(secret_bundle, m_db_key);
                LOG_INFO << "[Horizon Keyring] Secret encrypted with Database Key (AES-256-GCM).";
            } catch (const std::exception& e) {
                LOG_ERROR << "[Horizon Keyring] Encryption failed: " << e.what();
                return;
            }
        } else {
            LOG_ERROR << "[Horizon Keyring] Cannot create item: Keyring is locked!";
            m_dbus.send_error(msg, "org.freedesktop.Secret.Error.IsLocked", "The keyring must be unlocked before creating items.");
            return;
        }

        // Save to storage
        storage::SecretItem item;
        item.label = label;
        item.attributes = attributes;
        item.secret = final_data;
        m_storage->save_item("default", item);
        LOG_INFO << "[Horizon Keyring] Secret '" << label << "' saved to storage.";

        // Reply success
        m_dbus.send_reply(msg, {
            dbusutils::ObjectPath("/org/freedesktop/secrets/collection/default/1"),
            dbusutils::ObjectPath("/")
        });
    }

    void Service::handle_search_items(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: SearchItems";
        
        // Extract attributes from a{ss}
        std::map<std::string, std::string> attributes;
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        
        DBusMessageIter dict_iter;
        dbus_message_iter_recurse(&iter, &dict_iter);
        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter;
            dbus_message_iter_recurse(&dict_iter, &entry_iter);
            const char* key;
            const char* val;
            dbus_message_iter_get_basic(&entry_iter, &key);
            dbus_message_iter_next(&entry_iter);
            dbus_message_iter_get_basic(&entry_iter, &val);
            attributes[key] = val;
            dbus_message_iter_next(&dict_iter);
        }

        // Search in storage
        auto items = m_storage->search_items("default", attributes);
        
        std::vector<dbusutils::ObjectPath> unlocked_paths;
        for (const auto& item : items) {
            unlocked_paths.push_back(dbusutils::ObjectPath("/org/freedesktop/secrets/collection/default/" + item.path));
        }

        // Reply (ao unlocked, ao locked)
        m_dbus.send_reply(msg, {
            dbusutils::DbusVariant(unlocked_paths),
            dbusutils::DbusVariant(std::vector<dbusutils::ObjectPath>{}) // No locked paths for now
        });
    }

    void Service::handle_get_secrets(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: GetSecrets (Refined implementation)";
        
        if (m_master_key.empty()) {
            LOG_ERROR << "[Horizon Keyring] GetSecrets called but keyring is locked!";
            DBusMessage* reply = dbus_message_new_method_return(msg);
            DBusMessageIter root_iter, dict_iter;
            dbus_message_iter_init_append(reply, &root_iter);
            dbus_message_iter_open_container(&root_iter, DBUS_TYPE_ARRAY, "{o(oayays)}", &dict_iter);
            dbus_message_iter_close_container(&root_iter, &dict_iter);
            dbus_connection_send(m_dbus.get_connection(), reply, nullptr);
            dbus_message_unref(reply);
            return;
        }

        // 1. Extract arguments (ao, o)
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        
        std::vector<std::string> requested_paths;
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&iter, &array_iter);
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_OBJECT_PATH) {
            const char* path;
            dbus_message_iter_get_basic(&array_iter, &path);
            requested_paths.push_back(path);
            dbus_message_iter_next(&array_iter);
        }
        
        dbus_message_iter_next(&iter);
        const char* session_path;
        dbus_message_iter_get_basic(&iter, &session_path);

        // 2. Search for items
        auto all_items = m_storage->search_items("default", {});
        
        // 3. Build the response
        DBusMessage* reply = dbus_message_new_method_return(msg);
        DBusMessageIter root_iter, dict_iter;
        dbus_message_iter_init_append(reply, &root_iter);
        dbus_message_iter_open_container(&root_iter, DBUS_TYPE_ARRAY, "{o(oayays)}", &dict_iter);
        
        for (const auto& path : requested_paths) {
            // Extract index from path: /org/freedesktop/secrets/collection/default/ID
            size_t last_slash = path.find_last_of('/');
            if (last_slash == std::string::npos) continue;
            
            std::string id = path.substr(last_slash + 1);
            
            // Find item by ID
            auto it = std::find_if(all_items.begin(), all_items.end(), [&](const storage::SecretItem& item) {
                return item.path == id;
            });
            
            if (it == all_items.end()) continue;
            
            auto& item = *it;
            
            DBusMessageIter entry_iter;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            
            const char* p_path = path.c_str();
            dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_OBJECT_PATH, &p_path);
            
            DBusMessageIter struct_iter;
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_STRUCT, nullptr, &struct_iter);
            dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_OBJECT_PATH, &session_path);
            
            std::vector<uint8_t> decrypted;
            try {
                if (!m_db_key.empty()) {
                    decrypted = m_crypto->decrypt(item.secret, m_db_key);
                } else {
                    LOG_ERROR << "[Horizon Keyring] Keyring is locked, cannot decrypt items.";
                    decrypted = item.secret;
                }
            } catch (const std::exception& e) {
                LOG_ERROR << "[Horizon Keyring] Decryption failed for " << item.label << ": " << e.what();
                decrypted = item.secret; // Fallback
            }
            
            // Unbundle: [param_len] + [param_data] + [value_data]
            std::vector<uint8_t> param_data;
            std::vector<uint8_t> value_data;
            if (!decrypted.empty()) {
                uint8_t param_len = decrypted[0];
                if (1 + param_len <= decrypted.size()) {
                    param_data.assign(decrypted.begin() + 1, decrypted.begin() + 1 + param_len);
                    value_data.assign(decrypted.begin() + 1 + param_len, decrypted.end());
                } else {
                    value_data = decrypted; // Corrupted bundle, dump all
                }
            }
            
            DBusMessageIter param_iter;
            dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &param_iter);
            for (uint8_t b : param_data) {
                dbus_message_iter_append_basic(&param_iter, DBUS_TYPE_BYTE, &b);
            }
            dbus_message_iter_close_container(&struct_iter, &param_iter);
            
            DBusMessageIter secret_iter;
            dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &secret_iter);
            for (uint8_t b : value_data) {
                dbus_message_iter_append_basic(&secret_iter, DBUS_TYPE_BYTE, &b);
            }
            dbus_message_iter_close_container(&struct_iter, &secret_iter);
            
            const char* content_type = "text/plain";
            dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &content_type);
            
            dbus_message_iter_close_container(&entry_iter, &struct_iter);
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
        }
        
        dbus_message_iter_close_container(&root_iter, &dict_iter);
        dbus_connection_send(m_dbus.get_connection(), reply, nullptr);
        dbus_message_unref(reply);
    }

    void Service::handle_delete_item(DBusMessage* msg)
    {
        LOG_INFO << "[Horizon Keyring] Method call: DeleteItem";
        
        std::string path = dbus_message_get_path(msg) ? dbus_message_get_path(msg) : "";
        
        // Extract ID from path: /org/freedesktop/secrets/collection/default/<id>
        size_t last_slash = path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string id_str = path.substr(last_slash + 1);
            try {
                uint64_t item_id = std::stoull(id_str);
                bool deleted = m_storage->delete_item(item_id);
                if (deleted) {
                    LOG_INFO << "[Horizon Keyring] Deleted item " << item_id;
                } else {
                    LOG_WARNING << "[Horizon Keyring] Item " << item_id << " not found for deletion.";
                }
            } catch (const std::exception&) {
                LOG_ERROR << "[Horizon Keyring] Invalid item ID in path: " << path;
            }
        }

        // The Delete method returns a prompt path (o). We use "/" to indicate no prompt needed.
        m_dbus.send_reply(msg, {dbusutils::ObjectPath("/")});
    }

    void Service::handle_set_property(DBusMessage* msg)
    {
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        
        const char* interface_name;
        const char* property_name;
        dbus_message_iter_get_basic(&iter, &interface_name);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &property_name);
        dbus_message_iter_next(&iter);
        
        // The value is in a variant
        DBusMessageIter var_iter;
        dbus_message_iter_recurse(&iter, &var_iter);
        
        std::string path = dbus_message_get_path(msg);
        size_t last_slash = path.find_last_of('/');
        if (last_slash == std::string::npos) return;
        
        std::string id_str = path.substr(last_slash + 1);
        uint64_t item_id = 0;
        try { item_id = std::stoull(id_str); } catch (...) { return; }

        if (std::string(property_name) == "Label") {
            if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_STRING) {
                const char* label;
                dbus_message_iter_get_basic(&var_iter, &label);
                m_storage->update_item_label(item_id, label);
                LOG_INFO << "[Horizon Keyring] Updated label for item " << item_id << " to: " << label;
            }
        } else if (std::string(property_name) == "Attributes") {
            if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_ARRAY) {
                std::map<std::string, std::string> attributes;
                DBusMessageIter dict_iter;
                dbus_message_iter_recurse(&var_iter, &dict_iter);
                while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                    DBusMessageIter entry_iter;
                    dbus_message_iter_recurse(&dict_iter, &entry_iter);
                    const char* k;
                    const char* v;
                    dbus_message_iter_get_basic(&entry_iter, &k);
                    dbus_message_iter_next(&entry_iter);
                    dbus_message_iter_get_basic(&entry_iter, &v);
                    attributes[k] = v;
                    dbus_message_iter_next(&dict_iter);
                }
                m_storage->update_item_attributes(item_id, attributes);
                LOG_INFO << "[Horizon Keyring] Updated attributes for item " << item_id;
            }
        }

        m_dbus.send_reply(msg, {});
    }

    void Service::handle_get_property(DBusMessage* msg)
    {
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        
        const char* interface_name;
        const char* property_name;
        dbus_message_iter_get_basic(&iter, &interface_name);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &property_name);
        
        std::string path = dbus_message_get_path(msg) ? dbus_message_get_path(msg) : "";
        size_t last_slash = path.find_last_of('/');
        if (last_slash == std::string::npos) { m_dbus.send_reply(msg, {}); return; }
        
        std::string id_str = path.substr(last_slash + 1);
        auto all_items = m_storage->search_items("default", {});
        auto it = std::find_if(all_items.begin(), all_items.end(), [&](const storage::SecretItem& item) {
            return item.path == id_str;
        });
        
        if (it == all_items.end()) {
            m_dbus.send_reply(msg, {});
            return;
        }
        
        if (std::string(property_name) == "Label") {
            m_dbus.send_reply_custom(msg, {{true, it->label}});
        } else if (std::string(property_name) == "Attributes") {
            m_dbus.send_reply_custom(msg, {{true, it->attributes}});
        } else {
            m_dbus.send_reply(msg, {});
        }
    }

    void Service::handle_get_all_properties(DBusMessage* msg)
    {
        // For now, return an empty dictionary to avoid crashes if asked for all properties.
        // It's recommended to query specific properties via Get.
        m_dbus.send_reply(msg, {std::map<std::string, std::string>{}});
    }

    void Service::check_pending_unlock()
    {
        std::string path = "/tmp/horizon-pass-" + std::to_string(getuid());
        LOG_INFO << "[Horizon Keyring] Checking for pending password file at: " << path;
        
        if (access(path.c_str(), F_OK) == 0) {
            LOG_INFO << "[Horizon Keyring] Found pending password file. Attempting to read...";
            std::ifstream f(path);
            if (f.is_open()) {
                std::string password;
                if (std::getline(f, password)) {
                    LOG_INFO << "[Horizon Keyring] Password read successfully. Unlocking keyring...";
                    unlock_keyring(password);
                    f.close();
                    
                    // Securely delete the file
                    if (unlink(path.c_str()) == 0) {
                        LOG_INFO << "[Horizon Keyring] Pending password file deleted successfully.";
                    } else {
                        LOG_ERROR << "[Horizon Keyring] Failed to delete password file: " << strerror(errno);
                    }
                } else {
                    LOG_ERROR << "[Horizon Keyring] Failed to read password from file (empty?).";
                    f.close();
                }
            } else {
                LOG_ERROR << "[Horizon Keyring] Could not open password file for reading: " << strerror(errno);
            }
        } else {
            LOG_INFO << "[Horizon Keyring] No pending password file found (normal if already unlocked).";
        }
    }
}
