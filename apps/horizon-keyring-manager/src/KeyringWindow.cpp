#include "KeyringWindow.hpp"
#include <horizon/ToolbarButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>

namespace horizon::keyring
{
    KeyringWindow::KeyringWindow(int w, int h) : ApplicationWindow("Passwords and Keys")
    {
        set_size(w, h);
        setup_toolbar();
        setup_content();
        load_mock_data();
    }

    void KeyringWindow::setup_toolbar()
    {
        auto tb = toolbar();
        
        auto btn_add = std::make_unique<ToolbarButton>("", "list-add-symbolic");
        tb->add_toolbar_widget(std::move(btn_add));

        auto btn_del = std::make_unique<ToolbarButton>("", "edit-delete-symbolic");
        tb->add_toolbar_widget(std::move(btn_del));

        tb->add_toolbar_widget(Spacer());

        auto search = std::make_unique<SearchBox>();
        m_search_box = search.get();
        tb->add_toolbar_widget(std::move(search));
    }

    void KeyringWindow::setup_content()
    {
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);

        // Sidebar
        auto sidebar = std::make_unique<Sidebar>();
        m_sidebar = sidebar.get();
        sidebar->set_fixed_size(200);

        m_sidebar->add_group("Passwords");
        auto item_login = std::make_unique<SidebarItem>("Login", "folder-password-symbolic");
        auto item_pass = std::make_unique<SidebarItem>("Passwords", "dialog-password-symbolic");
        m_sidebar->add_item("Passwords", std::move(item_login));
        m_sidebar->add_item("Passwords", std::move(item_pass));

        m_sidebar->add_group("Keys");
        auto item_gpg = std::make_unique<SidebarItem>("GNUGpg Keys", "key-symbolic");
        auto item_ssh = std::make_unique<SidebarItem>("OpenSSH Keys", "key-symbolic");
        m_sidebar->add_item("Keys", std::move(item_gpg));
        m_sidebar->add_item("Keys", std::move(item_ssh));

        m_sidebar->add_group("Certificates");
        auto item_cert = std::make_unique<SidebarItem>("Default Trust", "certificate-symbolic");
        m_sidebar->add_item("Certificates", std::move(item_cert));

        // Table View
        auto table = std::make_unique<TableView<KeyringItem>>();
        m_table = table.get();

        m_table->add_column({
            "name", "Name", 250, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.label); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.label < b.label; }
        });
        
        m_table->add_column({
            "type", "Type", 100, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.type); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.type < b.type; }
        });

        m_table->add_column({
            "modified", "Modified", 150, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.last_modified); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.last_modified < b.last_modified; }
        });

        m_table->set_row_menu_factory([this](const KeyringItem& item) {
            auto menu = std::make_unique<Menu>();
            menu->add_item("Editar");
            menu->add_item("Eliminar");
            return menu;
        });

        container->add_child(std::move(sidebar));
        container->add_child(std::move(table));

        set_content(std::move(container));
    }

    void KeyringWindow::load_mock_data()
    {
        std::vector<KeyringItem> data = {
            {"Google Account", "Password", "2024-05-10", "/foo/1"},
            {"GitHub SSH Key", "SSH Key", "2024-05-12", "/foo/2"},
            {"Work VPN", "Password", "2024-05-11", "/foo/3"}
        };
        m_table->set_data(data);
    }
}
