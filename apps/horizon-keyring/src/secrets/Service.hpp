#pragma once

#include <horizon/dbusutils/DbusHelper.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../storage/StorageManager.hpp"
#include "../crypto/CryptoManager.hpp"
#include "SocketListener.hpp"

namespace horizon::secrets
{
    class Service : public dbusutils::DbusObject
    {
    public:
        explicit Service(dbusutils::DbusHelper& dbus);
        ~Service() override = default;

        DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) override;

    private:
        dbusutils::DbusHelper& m_dbus;
        std::unique_ptr<storage::StorageManager> m_storage;
        std::unique_ptr<crypto::CryptoManager> m_crypto;
        std::unique_ptr<SocketListener> m_socket_listener;
        std::vector<uint8_t> m_master_key;
        std::vector<uint8_t> m_db_key;

        void init_storage();
        void init_pam_listener();
        void unlock_keyring(const std::string& password);
        void handle_open_session(DBusMessage* msg);
        void handle_create_collection(DBusMessage* msg);
        void handle_search_items(DBusMessage* msg);
        void handle_get_secrets(DBusMessage* msg);
        void handle_read_alias(DBusMessage* msg);
        void handle_unlock(DBusMessage* msg);
        void handle_create_item(DBusMessage* msg);
        void handle_delete_item(DBusMessage* msg);
        void handle_set_property(DBusMessage* msg);
        void handle_get_property(DBusMessage* msg);
        void handle_get_all_properties(DBusMessage* msg);
        void check_pending_unlock();
    };
}
