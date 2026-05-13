#include "StorageManager.hpp"
#include <stdexcept>
#include <iostream>
#include <openssl/rand.h>

namespace horizon::secrets::storage
{
    StorageManager::StorageManager(const std::string& db_path) : m_path(db_path)
    {
        if (sqlite3_open(m_path.c_str(), &m_db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(m_db)));
        }
    }

    StorageManager::~StorageManager()
    {
        if (m_db) sqlite3_close(m_db);
    }

    void StorageManager::init_database()
    {
        execute_query("CREATE TABLE IF NOT EXISTS collections ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "name TEXT UNIQUE,"
                      "alias TEXT,"
                      "salt BLOB,"
                      "encrypted_master_key BLOB"
                      ");");

        execute_query("CREATE TABLE IF NOT EXISTS items ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "collection_id INTEGER, "
                      "label TEXT, "
                      "encrypted_secret BLOB, "
                      "FOREIGN KEY(collection_id) REFERENCES collections(id));");

        execute_query("CREATE TABLE IF NOT EXISTS attributes ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "item_id INTEGER, "
                      "key TEXT, "
                      "value TEXT, "
                      "FOREIGN KEY(item_id) REFERENCES items(id));");

        execute_query("CREATE TABLE IF NOT EXISTS keyring_meta ("
                      "key TEXT PRIMARY KEY, "
                      "value BLOB);");
    }

    std::vector<uint8_t> StorageManager::get_master_salt()
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(m_db, "SELECT value FROM keyring_meta WHERE key = 'master_salt';", -1, &stmt, nullptr);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Salt exists, return it
            const void* blob = sqlite3_column_blob(stmt, 0);
            int len = sqlite3_column_bytes(stmt, 0);
            std::vector<uint8_t> salt((const uint8_t*)blob, (const uint8_t*)blob + len);
            sqlite3_finalize(stmt);
            return salt;
        }
        sqlite3_finalize(stmt);

        // Salt doesn't exist, generate a new 16-byte random salt
        std::vector<uint8_t> new_salt(16);
        RAND_bytes(new_salt.data(), new_salt.size());

        // Save it to the database
        sqlite3_prepare_v2(m_db, "INSERT INTO keyring_meta (key, value) VALUES ('master_salt', ?);", -1, &stmt, nullptr);
        sqlite3_bind_blob(stmt, 1, new_salt.data(), (int)new_salt.size(), SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return new_salt;
    }

    void StorageManager::create_collection(const std::string& name, const std::string& alias)
    {
        sqlite3_stmt* stmt;
        const char* sql = "INSERT OR IGNORE INTO collections (name, alias) VALUES (?, ?);";
        sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, alias.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void StorageManager::execute_query(const std::string& query)
    {
        char* err_msg = nullptr;
        if (sqlite3_exec(m_db, query.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string err = err_msg;
            sqlite3_free(err_msg);
            throw std::runtime_error("SQL error: " + err);
        }
    }

    void StorageManager::save_item(const std::string& collection, const SecretItem& item)
    {
        // Get collection ID
        int coll_id = -1;
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(m_db, "SELECT id FROM collections WHERE name = ? OR alias = ?;", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, collection.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, collection.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            coll_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        if (coll_id == -1) return;

        // Insert item
        sqlite3_prepare_v2(m_db, "INSERT INTO items (collection_id, label, encrypted_secret) VALUES (?, ?, ?);", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, coll_id);
        sqlite3_bind_text(stmt, 2, item.label.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 3, item.secret.data(), (int)item.secret.size(), SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Get the ID of the inserted item
        sqlite3_int64 item_id = sqlite3_last_insert_rowid(m_db);

        // Insert attributes
        for (const auto& [key, value] : item.attributes) {
            sqlite3_prepare_v2(m_db, "INSERT INTO attributes (item_id, key, value) VALUES (?, ?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, item_id);
            sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    std::vector<SecretItem> StorageManager::search_items(const std::string& collection, const std::map<std::string, std::string>& attributes)
    {
        std::vector<SecretItem> results;
        sqlite3_stmt* stmt;

        // Base query
        std::string query = "SELECT DISTINCT items.id, items.label, items.encrypted_secret FROM items "
                            "JOIN collections ON items.collection_id = collections.id ";
        
        // Add attribute joins if needed
        int attr_count = 0;
        for (const auto& [k, v] : attributes) {
            query += "JOIN attributes AS a" + std::to_string(attr_count) + " ON items.id = a" + std::to_string(attr_count) + ".item_id ";
            attr_count++;
        }

        query += "WHERE (collections.name = ? OR collections.alias = ?) ";

        // Add attribute conditions
        attr_count = 0;
        for (const auto& [k, v] : attributes) {
            query += "AND a" + std::to_string(attr_count) + ".key = ? AND a" + std::to_string(attr_count) + ".value = ? ";
            attr_count++;
        }
        query += ";";

        sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, nullptr);
        
        int param_idx = 1;
        sqlite3_bind_text(stmt, param_idx++, collection.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, param_idx++, collection.c_str(), -1, SQLITE_STATIC);

        for (const auto& [k, v] : attributes) {
            sqlite3_bind_text(stmt, param_idx++, k.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, param_idx++, v.c_str(), -1, SQLITE_STATIC);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SecretItem item;
            item.path = std::to_string(sqlite3_column_int(stmt, 0));
            item.label = (const char*)sqlite3_column_text(stmt, 1);
            const void* blob = sqlite3_column_blob(stmt, 2);
            int blob_len = sqlite3_column_bytes(stmt, 2);
            item.secret.assign((const uint8_t*)blob, (const uint8_t*)blob + blob_len);
            results.push_back(item);
        }
        sqlite3_finalize(stmt);
        return results;
    }
}
