#pragma once
 
#include <horizon/ApplicationWindow.hpp>
#include "ItemDialog.hpp"
#include <horizon/Sidebar.hpp>
#include <horizon/TableView.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <string>
#include <vector>
 
namespace horizon::keyring
{
    struct KeyringItem
    {
        std::string label;
        std::string type;
        std::string last_modified;
        std::string path;
    };
 
    class KeyringWindow : public ApplicationWindow
    {
    public:
        KeyringWindow(int w, int h);
        ~KeyringWindow() override = default;
 
    private:
        void setup_toolbar();
        void setup_content();
        void load_data();
        void create_item_dialog();
        void edit_item_dialog(const KeyringItem& item);
        void delete_item(const std::string& path);
        void save_item(const std::string& label, const std::string& secret, const std::string& type, const std::string& existing_path = "");
        void handle_row_action(const std::string& action, const KeyringItem& item);
 
        Sidebar* m_sidebar{nullptr};
        TableView<KeyringItem>* m_table{nullptr};
        SearchBox* m_search_box{nullptr};
        std::string m_selected_sidebar_path{"All"};
        horizon::dbusutils::DbusHelper m_dbus{DBUS_BUS_SESSION};
    };
}
