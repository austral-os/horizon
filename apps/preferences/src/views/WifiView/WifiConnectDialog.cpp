#include <horizon/AquaObject.hpp>

#include <horizon/Spacer.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <iostream>
#include <uuid/uuid.h>
#include <views/WifiView/WifiConnectDialog.hpp>

namespace horizon::preferences
{
    static std::string generate_uuid()
    {
        uuid_t binuuid;
        uuid_generate_random(binuuid);
        char uuid_str[37];
        uuid_unparse_lower(binuuid, uuid_str);
        return std::string(uuid_str);
    }

    WifiConnectDialog::WifiConnectDialog(const std::string &ssid, const std::string &ap_path,
                                         const std::vector<WifiDevice> &devices)
        : WaylandWindow("horizon.wifi_connect", 450, 400, true, false), m_ssid(ssid),
          m_ap_path(ap_path), m_devices(devices)
    {
        set_name("Conectar a " + ssid);

        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (const std::exception &e)
        {
            std::cerr << "D-Bus init failed in dialog: " << e.what() << std::endl;
        }

        setup_ui(ssid);
    }

    void WifiConnectDialog::setup_ui(const std::string &ssid)
    {
        auto root_wnd = std::make_unique<Window>("Conectar a " + ssid);
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_margin(20);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(15);

        // 1. SSID Label
        auto label = std::make_unique<Label>("Introduce la clave para '" + ssid + "':");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_fixed_size(25);
        container->add_child(std::move(label));

        // 2. Device Combo
        auto device_label = std::make_unique<Label>("Dispositivo:");
        device_label->set_fixed_size(15);
        container->add_child(std::move(device_label));

        auto combo = std::make_unique<Combo>();
        for (const auto &dev : m_devices)
        {
            combo->add_item(dev.path, dev.name);
        }
        if (!m_devices.empty())
        {
            combo->set_selected_item_by_id(m_devices[0].path);
        }
        m_device_combo = combo.get();
        m_device_combo->set_fixed_size(35);
        container->add_child(std::move(combo));

        // 3. Password Input
        auto pass_label = std::make_unique<Label>("Contraseña:");
        pass_label->set_fixed_size(15);
        container->add_child(std::move(pass_label));

        auto input = std::make_unique<TextBox<PasswordPolicy>>();
        m_password_input = input.get();
        m_password_input->set_fixed_size(35);
        m_password_input->set_focusable(true);
        container->add_child(std::move(input));

        // 3.4 Loading Bar (New)
        auto loading_bar = std::make_unique<LoadingBar>();
        loading_bar->set_fixed_size(24);
        loading_bar->set_visible(false);
        m_loading_bar = loading_bar.get();
        container->add_child(std::move(loading_bar));

        // 3.5 Status Label (para informar errores o éxito)
        auto status_label = std::make_unique<Label>("No conectado a " + ssid);
        // status_label->set_font_size(13);
        status_label->set_alignment(TextAlignment::Center);
        m_status_label = status_label.get();
        container->add_child(std::move(status_label));

        container->add_child(Spacer());

        // 4. Buttons
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancelar");
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        m_cancel_btn = btn_cancel.get();
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text("Conectar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);

        m_accept_btn = btn_accept.get();

        btn_accept->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                if (m_accept_btn && m_accept_btn->text() == "Cerrar")
                {
                    this->quit();
                    return;
                }

                // Se capturan los datos en el UI Thread
                std::string password = m_password_input ? m_password_input->text() : "";
                const ComboItem *selected =
                    m_device_combo ? m_device_combo->selected_item() : nullptr;
                if (!selected)
                    return;
                std::string device_path = selected->id;

                if (m_status_label)
                {
                    m_status_label->set_text("Conectando...");
                    m_status_label->set_text_color(Color("#666666"));
                }

                if (m_loading_bar)
                {
                    m_loading_bar->set_visible(true);
                }

                // Deshabilitamos el UI para que no haya cambios mientras conectamos
                if (m_accept_btn)
                {
                    m_accept_btn->set_text("Conectando...");
                    m_accept_btn->set_enabled(false);
                }
                if (m_password_input)
                    m_password_input->set_enabled(false);
                if (m_device_combo)
                    m_device_combo->set_enabled(false);

                // Lanzamos el hilo trabajador para no congelar la ventana
                std::thread([this, password, device_path]()
                            { this->perform_connection_async(password, device_path); })
                    .detach();
            });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));
        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    // Helper macro-like lambda for appending a variant to a dict iterator
    auto append_variant =
        [](DBusMessageIter *dict_iter, const char *key, int type, const void *value)
    {
        DBusMessageIter entry_iter, var_iter;
        dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
        dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);

        char sig[2] = {(char)type, '\0'};
        dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, sig, &var_iter);
        dbus_message_iter_append_basic(&var_iter, type, value);
        dbus_message_iter_close_container(&entry_iter, &var_iter);

        dbus_message_iter_close_container(dict_iter, &entry_iter);
    };

#include <chrono>
#include <thread>

    void WifiConnectDialog::perform_connection_async(const std::string &password,
                                                     const std::string &device_path)
    {
        if (!m_dbus || !m_dbus->get_connection())
            return;

        std::string uuid = generate_uuid();

        DBusMessage *msg = dbus_message_new_method_call(
            "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
            "org.freedesktop.NetworkManager", "AddAndActivateConnection");
        if (!msg)
            return;

        DBusMessageIter iter, settings_iter;
        dbus_message_iter_init_append(msg, &iter);

        if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sa{sv}}", &settings_iter))
        {
            dbus_message_unref(msg);
            return;
        }

        // 1. "connection" group
        {
            DBusMessageIter conn_entry, conn_dict;
            const char *grp = "connection";
            dbus_message_iter_open_container(&settings_iter, DBUS_TYPE_DICT_ENTRY, NULL,
                                             &conn_entry);
            dbus_message_iter_append_basic(&conn_entry, DBUS_TYPE_STRING, &grp);
            dbus_message_iter_open_container(&conn_entry, DBUS_TYPE_ARRAY, "{sv}", &conn_dict);

            const char *type_str = "802-11-wireless";
            append_variant(&conn_dict, "type", DBUS_TYPE_STRING, &type_str);
            const char *ssid_id = m_ssid.c_str();
            append_variant(&conn_dict, "id", DBUS_TYPE_STRING, &ssid_id);
            const char *uuid_ptr = uuid.c_str();
            append_variant(&conn_dict, "uuid", DBUS_TYPE_STRING, &uuid_ptr);

            dbus_message_iter_close_container(&conn_entry, &conn_dict);
            dbus_message_iter_close_container(&settings_iter, &conn_entry);
        }

        // 2. "802-11-wireless" group
        {
            DBusMessageIter wifi_entry, wifi_dict;
            const char *grp = "802-11-wireless";
            dbus_message_iter_open_container(&settings_iter, DBUS_TYPE_DICT_ENTRY, NULL,
                                             &wifi_entry);
            dbus_message_iter_append_basic(&wifi_entry, DBUS_TYPE_STRING, &grp);
            dbus_message_iter_open_container(&wifi_entry, DBUS_TYPE_ARRAY, "{sv}", &wifi_dict);

            {
                DBusMessageIter entry_iter, var_iter, array_iter;
                const char *key = "ssid";
                dbus_message_iter_open_container(&wifi_dict, DBUS_TYPE_DICT_ENTRY, NULL,
                                                 &entry_iter);
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
                dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "ay", &var_iter);
                dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "y", &array_iter);

                for (size_t i = 0; i < m_ssid.size(); ++i)
                {
                    uint8_t b = static_cast<uint8_t>(m_ssid[i]);
                    dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_BYTE, &b);
                }

                dbus_message_iter_close_container(&var_iter, &array_iter);
                dbus_message_iter_close_container(&entry_iter, &var_iter);
                dbus_message_iter_close_container(&wifi_dict, &entry_iter);
            }

            const char *mode_str = "infrastructure";
            append_variant(&wifi_dict, "mode", DBUS_TYPE_STRING, &mode_str);
            dbus_message_iter_close_container(&wifi_entry, &wifi_dict);
            dbus_message_iter_close_container(&settings_iter, &wifi_entry);
        }

        // 3. "802-11-wireless-security" group
        if (!password.empty())
        {
            DBusMessageIter sec_entry, sec_dict;
            const char *grp = "802-11-wireless-security";
            dbus_message_iter_open_container(&settings_iter, DBUS_TYPE_DICT_ENTRY, NULL,
                                             &sec_entry);
            dbus_message_iter_append_basic(&sec_entry, DBUS_TYPE_STRING, &grp);
            dbus_message_iter_open_container(&sec_entry, DBUS_TYPE_ARRAY, "{sv}", &sec_dict);

            const char *key_mgmt = "wpa-psk";
            append_variant(&sec_dict, "key-mgmt", DBUS_TYPE_STRING, &key_mgmt);
            const char *psk = password.c_str();
            append_variant(&sec_dict, "psk", DBUS_TYPE_STRING, &psk);

            dbus_message_iter_close_container(&sec_entry, &sec_dict);
            dbus_message_iter_close_container(&settings_iter, &sec_entry);
        }

        // 4. "ipv4" and "ipv6"
        {
            auto add_ip = [&](const char *name, const char *method_val)
            {
                DBusMessageIter ent, dic;
                dbus_message_iter_open_container(&settings_iter, DBUS_TYPE_DICT_ENTRY, NULL, &ent);
                dbus_message_iter_append_basic(&ent, DBUS_TYPE_STRING, &name);
                dbus_message_iter_open_container(&ent, DBUS_TYPE_ARRAY, "{sv}", &dic);
                append_variant(&dic, "method", DBUS_TYPE_STRING, &method_val);
                dbus_message_iter_close_container(&ent, &dic);
                dbus_message_iter_close_container(&settings_iter, &ent);
            };
            add_ip("ipv4", "auto");
            add_ip("ipv6", "ignore");
        }

        dbus_message_iter_close_container(&iter, &settings_iter);

        const char *dev_p = device_path.c_str();
        const char *ap_p = m_ap_path.c_str();
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &dev_p);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &ap_p);

        DBusError dbus_err;
        dbus_error_init(&dbus_err);
        DBusMessage *reply =
            dbus_connection_send_with_reply_and_block(m_dbus->get_connection(), msg, -1, &dbus_err);

        if (dbus_error_is_set(&dbus_err) || reply == nullptr)
        {
            std::string err_m = (dbus_error_is_set(&dbus_err) ? dbus_err.message : "Desconocido");
            if (m_accept_btn)
            {
                m_accept_btn->set_enabled(true);
                m_accept_btn->set_text("Reintentar");
            }
            if (m_status_label)
            {
                m_status_label->set_text_color(Color("#ff4444"));
                m_status_label->set_text("No conectado a " + m_ssid);
            }
            if (m_password_input)
                m_password_input->set_enabled(true);
            if (m_device_combo)
                m_device_combo->set_enabled(true);
            if (m_loading_bar)
                m_loading_bar->set_visible(false);
            dbus_error_free(&dbus_err);
            dbus_message_unref(msg);
            return;
        }

        auto paths = m_dbus->get_all_object_paths(reply);
        std::string active_conn_path = (paths.size() >= 2) ? paths[1] : "";
        dbus_message_unref(reply);
        dbus_message_unref(msg);

        if (active_conn_path.empty())
        {
            this->quit();
            return;
        }

        bool success = false;
        bool finished = false;
        int attempts = 0;
        while (!finished && attempts < 40)
        { // Max 20 segundos
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            attempts++;

            auto state_var =
                m_dbus->get_property("org.freedesktop.NetworkManager", active_conn_path,
                                     "org.freedesktop.NetworkManager.Connection.Active", "State");

            if (std::holds_alternative<uint32_t>(state_var))
            {
                uint32_t state = std::get<uint32_t>(state_var);
                if (state == 2)
                {
                    success = true;
                    finished = true;
                }
                else if (state == 4)
                {
                    success = false;
                    finished = true;
                }
            }
        }

        if (finished)
        {
            if (success)
            {
                if (m_status_label)
                {
                    m_status_label->set_text_color(Color("#44aa44"));
                    m_status_label->set_text("Conectado a " + m_ssid);
                }
                if (m_accept_btn)
                {
                    m_accept_btn->set_enabled(true);
                    m_accept_btn->set_text("Cerrar");
                }
                if (m_loading_bar)
                    m_loading_bar->set_visible(false);
            }
            else
            {
                if (m_status_label)
                {
                    m_status_label->set_text_color(Color("#ff4444"));
                    m_status_label->set_text("No conectado a " + m_ssid);
                }
                if (m_accept_btn)
                {
                    m_accept_btn->set_enabled(true);
                    m_accept_btn->set_text("Reintentar");
                }
                if (m_password_input)
                    m_password_input->set_enabled(true);
                if (m_device_combo)
                    m_device_combo->set_enabled(true);
                if (m_loading_bar)
                    m_loading_bar->set_visible(false);
            }
        }
        else
        {
            if (m_status_label)
            {
                m_status_label->set_text_color(Color("#ffaa00"));
                m_status_label->set_text("No conectado a " + m_ssid + " (Tiempo agotado)");
            }
            if (m_accept_btn)
            {
                m_accept_btn->set_enabled(true);
                m_accept_btn->set_text("Reintentar");
            }
            if (m_password_input)
                m_password_input->set_enabled(true);
            if (m_device_combo)
                m_device_combo->set_enabled(true);
            if (m_loading_bar)
                m_loading_bar->set_visible(false);
        }
    }
} // namespace horizon::preferences
