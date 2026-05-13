#include "KeyringWindow.hpp"
#include <horizon/ToolbarButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <horizon/ToggleGroupButton.hpp>
#include <horizon/I18n.hpp>

namespace horizon::keyring
{
    KeyringWindow::KeyringWindow(int w, int h) : ApplicationWindow("Passwords and Keys")
    {
        set_size(w, h);
        setup_toolbar();
        setup_content();
        load_data();
    }

    void KeyringWindow::setup_toolbar()
    {
        auto tb = toolbar();
        
        // Add Button
        auto btn_add = std::make_unique<ToolbarButton>("Añadir", "list-add-symbolic");
        tb->add_toolbar_widget(std::move(btn_add));

        // Delete Button
        auto btn_del = std::make_unique<ToolbarButton>("Eliminar", "edit-delete-symbolic");
        btn_del->when_click.connect([this](EventContext&) {
            auto items = m_table->get_selected_items();
            for (const auto& item : items) {
                delete_item(item.path);
            }
        });
        tb->add_toolbar_widget(std::move(btn_del));

        // Refresh Button
        auto btn_refresh = std::make_unique<ToolbarButton>("Actualizar", "view-refresh-symbolic");
        btn_refresh->when_click.connect([this](EventContext&) { load_data(); });
        tb->add_toolbar_widget(std::move(btn_refresh));

        tb->add_toolbar_widget(Spacer());

        // Search Box with Wrapper
        auto search_box = std::make_unique<SearchBox>();
        m_search_box = search_box.get();
        m_search_box->set_placeholder("Buscar...");
        m_search_box->set_fixed_size(35);
        m_search_box->when_text_changed.connect([this](KeyEventContext &) { 
            load_data(); 
        });

        auto search_wrapper = std::make_unique<Widget>();
        search_wrapper->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        search_wrapper->set_fixed_size(200);
        search_wrapper->add_child(std::move(search_box));
        
        tb->add_toolbar_widget(std::move(search_wrapper));
        tb->add_toolbar_widget(Spacer(10));
    }

    void KeyringWindow::setup_content()
    {
        auto vpanel = std::make_unique<VPanel>();
        vpanel->set_left_width(200);

        // Sidebar
        auto sidebar = std::make_unique<Sidebar>();
        m_sidebar = sidebar.get();

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
            auto* del = menu->add_item("Eliminar");
            del->when_click.connect([this, item](EventContext&) {
                handle_row_action("delete", item);
            });
            return menu;
        });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(table));
        
        set_content(std::move(vpanel));
    }

    void KeyringWindow::load_data()
    {
        std::vector<KeyringItem> data;
        std::map<std::string, std::string> empty_attrs;
        std::vector<dbusutils::DbusVariant> args;
        args.push_back(empty_attrs);

        DBusMessage* reply = m_dbus.call_method_sync(
            "org.freedesktop.secrets",
            "/org/freedesktop/secrets",
            "org.freedesktop.Secret.Service",
            "SearchItems",
            args,
            1000
        );

        if (reply) {
            DBusMessageIter iter;
            dbus_message_iter_init(reply, &iter);
            
            if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&iter, &array_iter);
                while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_OBJECT_PATH) {
                    const char* path;
                    dbus_message_iter_get_basic(&array_iter, &path);
                    
                    std::string p(path);
                    auto label_var = m_dbus.get_property("org.freedesktop.secrets", p, "org.freedesktop.Secret.Item", "Label");
                    std::string label = "Unknown";
                    if (std::holds_alternative<std::string>(label_var)) {
                        label = std::get<std::string>(label_var);
                    }
                    
                    auto attrs_var = m_dbus.get_property("org.freedesktop.secrets", p, "org.freedesktop.Secret.Item", "Attributes");
                    std::string type = "Password";
                    if (std::holds_alternative<std::map<std::string, std::string>>(attrs_var)) {
                        auto attrs = std::get<std::map<std::string, std::string>>(attrs_var);
                        if (attrs.count("type")) type = attrs["type"];
                    }

                    data.push_back({label, type, "Now", p});
                    dbus_message_iter_next(&array_iter);
                }
            }
            dbus_message_unref(reply);
        }

        std::string filter = m_search_box ? m_search_box->text() : "";
        std::vector<KeyringItem> filtered;
        for (const auto& item : data) {
            if (filter.empty() || 
                item.label.find(filter) != std::string::npos ||
                item.type.find(filter) != std::string::npos) {
                filtered.push_back(item);
            }
        }
        m_table->set_data(std::move(filtered));
    }

    void KeyringWindow::delete_item(const std::string& path)
    {
        m_dbus.call_method_void(
            "org.freedesktop.secrets",
            path,
            "org.freedesktop.Secret.Item",
            "Delete"
        );
        load_data();
    }

    void KeyringWindow::handle_row_action(const std::string& action, const KeyringItem& item)
    {
        if (action == "delete") {
            delete_item(item.path);
        }
    }
}
