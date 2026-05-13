#include "KeyringWindow.hpp"
#include <horizon/ToolbarButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>

namespace horizon::keyring
{
    KeyringWindow::KeyringWindow(int w, int h) : ApplicationWindow(i18n().tr("keyring.title"))
    {
        set_size(w, h);
        setup_toolbar();
        setup_content();
        setup_status_bar();
        load_data();
    }

    void KeyringWindow::setup_toolbar()
    {
        auto tb = toolbar();
        
        // Add Button
        auto btn_add = std::make_unique<ToolbarButton>(i18n().tr("keyring.toolbar.add"), "list-add-symbolic");
        btn_add->when_click.connect([this](EventContext&) {
            create_item_dialog();
        });
        tb->add_toolbar_widget(std::move(btn_add));

        // Delete Button
        auto btn_del = std::make_unique<ToolbarButton>(i18n().tr("keyring.toolbar.delete"), "edit-delete-symbolic");
        btn_del->when_click.connect([this](EventContext&) {
            auto items = m_table->get_selected_items();
            if (items.empty()) return;

            std::string msg = (items.size() == 1) 
                ? i18n().tr("keyring.dialog.confirm_del_single", {{"name", items[0].label}})
                : i18n().tr("keyring.dialog.confirm_del_multiple", {{"count", std::to_string(items.size())}});

            if (application()->confirm(msg, i18n().tr("keyring.dialog.confirm_title"))) {
                for (const auto& item : items) {
                    delete_item(item.path);
                }
                load_data();
            }
        });
        tb->add_toolbar_widget(std::move(btn_del));

        // Refresh Button
        auto btn_refresh = std::make_unique<ToolbarButton>(i18n().tr("keyring.toolbar.refresh"), "view-refresh-symbolic");
        btn_refresh->when_click.connect([this](EventContext&) { load_data(); });
        tb->add_toolbar_widget(std::move(btn_refresh));

        tb->add_toolbar_widget(Spacer());

        // Search Box with Wrapper
        auto search_box = std::make_unique<SearchBox>();
        m_search_box = search_box.get();
        m_search_box->set_placeholder(i18n().tr("keyring.toolbar.search"));
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

        m_sidebar->add_group(i18n().tr("keyring.sidebar.passwords"));
        auto item_login = std::make_unique<SidebarItem>("folder-password-symbolic", i18n().tr("keyring.sidebar.login"));
        item_login->set_path("Passwords");
        auto item_pass = std::make_unique<SidebarItem>("dialog-password-symbolic", i18n().tr("keyring.sidebar.passwords"));
        item_pass->set_path("Passwords");
        m_sidebar->add_item(i18n().tr("keyring.sidebar.passwords"), std::move(item_login));
        m_sidebar->add_item(i18n().tr("keyring.sidebar.passwords"), std::move(item_pass));
        
        m_sidebar->add_group(i18n().tr("keyring.sidebar.keys"));
        auto item_gpg = std::make_unique<SidebarItem>("key-symbolic", i18n().tr("keyring.sidebar.gnupg"));
        item_gpg->set_path("Keys");
        auto item_ssh = std::make_unique<SidebarItem>("key-symbolic", i18n().tr("keyring.sidebar.openssh"));
        item_ssh->set_path("Keys");
        m_sidebar->add_item(i18n().tr("keyring.sidebar.keys"), std::move(item_gpg));
        m_sidebar->add_item(i18n().tr("keyring.sidebar.keys"), std::move(item_ssh));
        
        m_sidebar->add_group(i18n().tr("keyring.sidebar.certificates"));
        auto item_cert = std::make_unique<SidebarItem>("certificate-symbolic", i18n().tr("keyring.sidebar.default_trust"));
        item_cert->set_path("Certificates");
        m_sidebar->add_item(i18n().tr("keyring.sidebar.certificates"), std::move(item_cert));

        m_sidebar->when_item_selected.connect([this](SidebarItemSelectedContext &ctx) {
            if (ctx.item) {
                m_selected_sidebar_path = ctx.item->path();
                load_data();
            }
        });

        // Table View
        auto table = std::make_unique<TableView<KeyringItem>>();
        m_table = table.get();

        m_table->add_column({
            "name", i18n().tr("keyring.table.name"), 250, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.label); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.label < b.label; }
        });
        
        m_table->add_column({
            "type", i18n().tr("keyring.table.type"), 100, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.type); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.type < b.type; }
        });

        m_table->add_column({
            "modified", i18n().tr("keyring.table.modified"), 150, ColumnWidthPolicy::Fixed, true,
            [](const KeyringItem& item) { return std::make_unique<Label>(item.last_modified); },
            [](const KeyringItem& a, const KeyringItem& b) { return a.last_modified < b.last_modified; }
        });

        m_table->set_row_menu_factory([this](const KeyringItem& item) {
            auto menu = std::make_unique<Menu>();
            auto* edit = menu->add_item(i18n().tr("keyring.menu.edit"));
            edit->when_click.connect([this, item](EventContext&) {
                handle_row_action("edit", item);
            });

            menu->add_separator();

            auto* del = menu->add_item(i18n().tr("keyring.menu.delete"));
            del->when_click.connect([this, item](EventContext&) {
                handle_row_action("delete", item);
            });
            return menu;
        });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(table));
        
        set_content(std::move(vpanel));
    }

    void KeyringWindow::setup_status_bar()
    {
        show_status_bar();
        auto* sb = statusbar();
        auto lbl = std::make_unique<Label>("");
        m_status_label = lbl.get();
        m_status_label->set_alignment(TextAlignment::Left);
        sb->add_child(Spacer(10));
        sb->add_child(std::move(lbl));
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
            // Sidebar filter
            if (m_selected_sidebar_path != "All" && !m_selected_sidebar_path.empty()) {
                bool match = false;
                if (m_selected_sidebar_path == "Passwords") match = (item.type == "Password");
                else if (m_selected_sidebar_path == "Keys") match = (item.type == "Key" || item.type == "SSH Key");
                else if (m_selected_sidebar_path == "Certificates") match = (item.type == "Certificate");
                
                if (!match) continue;
            }

            // Search filter
            if (filter.empty() || 
                item.label.find(filter) != std::string::npos ||
                item.type.find(filter) != std::string::npos) {
                filtered.push_back(item);
            }
        }
        size_t count = filtered.size();
        m_table->set_data(std::move(filtered));
        if (m_status_label) {
            m_status_label->set_text(i18n().tr("keyring.status.elements", {{"count", std::to_string(count)}}));
        }
    }

    void KeyringWindow::delete_item(const std::string& path)
    {
        m_dbus.call_method_void(
            "org.freedesktop.secrets",
            path,
            "org.freedesktop.Secret.Item",
            "Delete"
        );
    }

    void KeyringWindow::create_item_dialog()
    {
        application()->post_task([this]() {
            auto dialog = std::make_unique<ItemDialog>(i18n().tr("keyring.dialog.create_title"));
            dialog->when_accepted.connect([this](ItemEvent& ev) {
                save_item(ev.label, ev.secret, ev.type);
            });
            dialog->run();
        });
    }

    void KeyringWindow::edit_item_dialog(const KeyringItem& item)
    {
        application()->post_task([this, item]() {
            auto dialog = std::make_unique<ItemDialog>(i18n().tr("keyring.dialog.edit_title"));
            dialog->set_initial_values(item.label, "", item.type);
            dialog->when_accepted.connect([this, item](ItemEvent& ev) {
                save_item(ev.label, ev.secret, ev.type, item.path);
            });
            dialog->run();
        });
    }

    void KeyringWindow::save_item(const std::string& label, const std::string& secret, const std::string& type, const std::string& existing_path)
    {
        if (existing_path.empty()) {
            DBusMessage* msg = dbus_message_new_method_call(
                "org.freedesktop.secrets",
                "/org/freedesktop/secrets/collection/default",
                "org.freedesktop.Secret.Collection",
                "CreateItem"
            );

            DBusMessageIter iter;
            dbus_message_iter_init_append(msg, &iter);

            // 1. Properties a{sv}
            DBusMessageIter dict_iter;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

            // Label
            {
                DBusMessageIter entry_iter;
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                const char* key = "org.freedesktop.Secret.Item.Label";
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
                DBusMessageIter var_iter;
                const char* val = label.c_str();
                dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &var_iter);
                dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&entry_iter, &var_iter);
                dbus_message_iter_close_container(&dict_iter, &entry_iter);
            }

            // Attributes
            {
                DBusMessageIter entry_iter;
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                const char* key = "org.freedesktop.Secret.Item.Attributes";
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
                DBusMessageIter var_iter;
                dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "a{ss}", &var_iter);
                DBusMessageIter attr_dict_iter;
                dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "{ss}", &attr_dict_iter);
                
                // Add "type" attribute
                DBusMessageIter attr_entry_iter;
                dbus_message_iter_open_container(&attr_dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &attr_entry_iter);
                const char* k = "type";
                const char* v = type.c_str();
                dbus_message_iter_append_basic(&attr_entry_iter, DBUS_TYPE_STRING, &k);
                dbus_message_iter_append_basic(&attr_entry_iter, DBUS_TYPE_STRING, &v);
                dbus_message_iter_close_container(&attr_dict_iter, &attr_entry_iter);
                
                dbus_message_iter_close_container(&var_iter, &attr_dict_iter);
                dbus_message_iter_close_container(&entry_iter, &var_iter);
                dbus_message_iter_close_container(&dict_iter, &entry_iter);
            }

            dbus_message_iter_close_container(&iter, &dict_iter);

            // 2. Secret (oayays)
            DBusMessageIter struct_iter;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &struct_iter);
            
            // Session (o)
            const char* session = "/org/freedesktop/secrets/session/plain";
            dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_OBJECT_PATH, &session);
            
            // Parameters (ay)
            DBusMessageIter param_iter;
            dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &param_iter);
            dbus_message_iter_close_container(&struct_iter, &param_iter);
            
            // Secret value (ay)
            DBusMessageIter val_iter;
            dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &val_iter);
            for (char c : secret) {
                dbus_message_iter_append_basic(&val_iter, DBUS_TYPE_BYTE, &c);
            }
            dbus_message_iter_close_container(&struct_iter, &val_iter);
            
            // Content type (s)
            const char* content_type = "text/plain";
            dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &content_type);
            
            dbus_message_iter_close_container(&iter, &struct_iter);

            // 3. Replace (b)
            dbus_bool_t replace = true;
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &replace);

            DBusError err;
            dbus_error_init(&err);
            DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_dbus.get_connection(), msg, 1000, &err);
            
            if (dbus_error_is_set(&err)) {
                LOG_ERROR << "Error creating item: " << err.message;
                dbus_error_free(&err);
            }
            
            if (reply) dbus_message_unref(reply);
            dbus_message_unref(msg);
        } else {
            // EDIT
            // Update Label
            m_dbus.call_method_sync(
                "org.freedesktop.secrets",
                existing_path,
                "org.freedesktop.DBus.Properties",
                "Set",
                { std::string("org.freedesktop.Secret.Item"), std::string("Label"), dbusutils::DbusVariant(label) }
            );

            // Update Attributes
            std::map<std::string, std::string> attrs;
            attrs["type"] = type;
            m_dbus.call_method_sync(
                "org.freedesktop.secrets",
                existing_path,
                "org.freedesktop.DBus.Properties",
                "Set",
                { std::string("org.freedesktop.Secret.Item"), std::string("Attributes"), dbusutils::DbusVariant(attrs) }
            );
        }
        load_data();
    }

    void KeyringWindow::handle_row_action(const std::string& action, const KeyringItem& item)
    {
        if (action == "delete") {
            if (application()->confirm(i18n().tr("keyring.dialog.confirm_del_single", {{"name", item.label}}), i18n().tr("keyring.dialog.confirm_title"))) {
                delete_item(item.path);
                load_data();
            }
        } else if (action == "edit") {
            edit_item_dialog(item);
        }
    }
}
