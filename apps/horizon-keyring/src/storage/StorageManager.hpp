#pragma once

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include <cstdint>

namespace horizon::secrets::storage
{
    struct SecretItem
    {
        std::string path;
        std::string label;
        std::vector<uint8_t> secret;
        std::map<std::string, std::string> attributes;
    };

    class StorageManager
    {
    public:
        StorageManager(const std::string& db_path);
        ~StorageManager();

        void init_database();

        void create_collection(const std::string& name, const std::string& alias = "");

        // Master Key Salt Management
        std::vector<uint8_t> get_master_salt();

        // Item management
        void save_item(const std::string& collection, const SecretItem& item);
        bool delete_item(uint64_t item_id);
        void update_item_label(uint64_t item_id, const std::string& label);
        void update_item_attributes(uint64_t item_id, const std::map<std::string, std::string>& attributes);
        std::vector<SecretItem> search_items(const std::string& collection, const std::map<std::string, std::string>& attributes);

    private:
        sqlite3* m_db{nullptr};
        std::string m_path;

        void execute_query(const std::string& query);
    };
}
