#include <algorithm>
#include <cstdio>
#include <horizon/Icon.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/WaylandWindow.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <views/BluetoothView/BluetoothPairDialog.hpp>
#include <views/BluetoothView/BluetoothView.hpp>

namespace horizon::preferences
{
    BluetoothView::BluetoothView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(10);
        set_margin(10);

        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to initialize D-Bus in BluetoothView: " << e.what() << std::endl;
        }

        setup_ui();
        refresh_devices();

        when_application_load.connect(
            [this](EventContext &)
            {
                if (auto *app = application())
                {
                    app->add_timer(1000, [this]() { m_initialized = true; });

                    // Start monitoring thread
                    m_stop_monitor = false;
                    m_monitor_thread = std::thread(&BluetoothView::monitor_loop, this);
                }
            });
    }

    BluetoothView::~BluetoothView()
    {
        m_stop_monitor = true;
        if (m_monitor_thread.joinable())
        {
            m_monitor_thread.join();
        }

        if (m_refresh_timer_id != 0)
        {
            if (auto *app = application())
            {
                app->stop_timer(m_refresh_timer_id);
            }
        }
    }

    void BluetoothView::monitor_loop()
    {
        DBusError err;
        dbus_error_init(&err);

        DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
        if (dbus_error_is_set(&err))
        {
            std::cerr << "BluetoothView Monitor: D-Bus connection error: " << err.message
                      << std::endl;
            dbus_error_free(&err);
            return;
        }

        // Match common BlueZ signals
        dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.DBus.Properties'", &err);
        dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.DBus.ObjectManager'",
                           &err);

        if (dbus_error_is_set(&err))
        {
            std::cerr << "Error matching Bluetooth signals" << std::endl;
            dbus_error_free(&err);
        }

        while (!m_stop_monitor)
        {
            // Read messages (non-blocking wait)
            dbus_connection_read_write_dispatch(conn, 100);

            DBusMessage *msg = dbus_connection_pop_message(conn);
            if (!msg)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // If we get an ObjectManager or PropertiesChanged from BlueZ, refresh the list
            if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                                       "InterfacesAdded") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                                       "InterfacesRemoved") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
            {
                const char *sender = dbus_message_get_sender(msg);
                if (sender && std::string(sender).find("bluez") != std::string::npos ||
                    dbus_message_has_interface(msg, "org.bluez.Device1")) // Simplification
                {
                    if (auto *app = application())
                    {
                        app->post_task([this]() { this->refresh_devices(); });
                    }
                }
            }

            dbus_message_unref(msg);
        }

        dbus_connection_unref(conn);
    }

    void BluetoothView::setup_ui()
    {
        // 1. Label: Dispositivos Vinculados
        auto label = std::make_unique<Label>("Dispositivos Vinculados");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_fixed_size(24);
        m_title_label = label.get();
        add_child(std::move(label));

        // 2. TableView: Name, Connection Status
        auto table = std::make_unique<TableView<BluetoothDevice>>();

        TableColumn<BluetoothDevice> name_col;
        name_col.title = "Nombre del dispositivo";
        name_col.width = 300;
        name_col.cell_factory = [](const BluetoothDevice &data)
        { return std::make_unique<Label>(data.name.empty() ? data.address : data.name); };
        table->add_column(std::move(name_col));

        TableColumn<BluetoothDevice> status_col;
        status_col.title = "Estado";
        status_col.width = 150;
        status_col.cell_factory = [](const BluetoothDevice &data)
        {
            auto lbl = std::make_unique<Label>(data.connected ? "Conectado" : "Desconectado");
            if (data.connected)
            {
                lbl->set_text_color(Color("#0b7c37ff")); // Emerald Green
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
            }
            return lbl;
        };
        table->add_column(std::move(status_col));

        m_table_view = table.get();
        m_table_view->when_row_click.connect([this](auto &ctx)
                                             { this->on_device_selected(ctx.row_data); });
        add_child(std::move(table));

        // 3. Buttons: Sync New, Connect/Disconnect, Refresh (Horizontal Container)
        auto button_container = std::make_unique<Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(35);
        button_container->set_spacing(10);

        auto sync_btn = std::make_unique<Button<SolidObject>>();
        sync_btn->set_text("Sincronizar nuevo");
        sync_btn->when_click.connect([this](MouseButtonEventContext &)
                                     { this->on_sync_new_clicked(); });
        m_sync_button = sync_btn.get();
        button_container->add_child(std::move(sync_btn));

        auto connect_btn = std::make_unique<Button<SolidObject>>();
        connect_btn->set_text("Conectar");
        connect_btn->set_enabled(false);
        connect_btn->when_click.connect([this](MouseButtonEventContext &)
                                        { this->on_connect_clicked(); });
        m_connect_button = connect_btn.get();
        button_container->add_child(std::move(connect_btn));

        auto refresh_btn = std::make_unique<Button<SolidObject>>();
        refresh_btn->set_text("Refrescar");
        refresh_btn->when_click.connect([this](MouseButtonEventContext &)
                                        { this->refresh_devices(); });
        m_refresh_button = refresh_btn.get();
        button_container->add_child(std::move(refresh_btn));

        button_container->add_child(Spacer());
        add_child(std::move(button_container));
    }

    void BluetoothView::refresh_devices()
    {
        if (m_table_view)
        {
            m_table_view->set_data(get_paired_devices());

            // Check if selected device is still there
            if (!m_selected_device.address.empty())
            {
                bool found = false;
                auto devices = m_table_view->data();
                for (auto &d : devices)
                {
                    if (d.address == m_selected_device.address)
                    {
                        m_selected_device = d;
                        found = true;
                        break;
                    }
                }
                if (m_connect_button)
                {
                    m_connect_button->set_text(m_selected_device.connected ? "Desconectar"
                                                                           : "Conectar");
                }
                if (!found && m_connect_button)
                {
                    m_connect_button->set_enabled(false);
                }
            }

            if (auto *app = application())
            {
                if (m_refresh_timer_id != 0)
                    app->stop_timer(m_refresh_timer_id);
                m_refresh_timer_id = app->add_timer(10000,
                                                    [this]()
                                                    {
                                                        m_refresh_timer_id = 0;
                                                        this->refresh_devices();
                                                    });
            }
        }
    }

    std::vector<BluetoothDevice> BluetoothView::get_paired_devices()
    {
        std::vector<BluetoothDevice> devices;
        if (!m_dbus)
            return devices;

        DBusMessage *msg = m_dbus->call_method(
            "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        if (!msg)
            return devices;

        DBusMessageIter iter, dict_iter;
        if (!dbus_message_iter_init(msg, &iter))
        {
            dbus_message_unref(msg);
            return devices;
        }

        dbus_message_iter_recurse(&iter, &dict_iter);
        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY)
        {
            DBusMessageIter entry_iter, path_iter, interfaces_iter;
            dbus_message_iter_recurse(&dict_iter, &entry_iter);

            const char *object_path;
            dbus_message_iter_get_basic(&entry_iter, &object_path);
            dbus_message_iter_next(&entry_iter);
            dbus_message_iter_recurse(&entry_iter, &interfaces_iter);

            while (dbus_message_iter_get_arg_type(&interfaces_iter) == DBUS_TYPE_DICT_ENTRY)
            {
                DBusMessageIter interface_entry, properties_iter;
                dbus_message_iter_recurse(&interfaces_iter, &interface_entry);

                const char *interface_name;
                dbus_message_iter_get_basic(&interface_entry, &interface_name);

                if (std::string(interface_name) == "org.bluez.Device1")
                {
                    dbus_message_iter_next(&interface_entry);
                    dbus_message_iter_recurse(&interface_entry, &properties_iter);

                    BluetoothDevice dev;
                    dev.path = object_path;
                    dev.paired = false;
                    dev.connected = false;

                    while (dbus_message_iter_get_arg_type(&properties_iter) == DBUS_TYPE_DICT_ENTRY)
                    {
                        DBusMessageIter prop_entry, variant_iter;
                        dbus_message_iter_recurse(&properties_iter, &prop_entry);

                        const char *prop_name;
                        dbus_message_iter_get_basic(&prop_entry, &prop_name);
                        dbus_message_iter_next(&prop_entry);
                        dbus_message_iter_recurse(&prop_entry, &variant_iter);

                        if (std::string(prop_name) == "Name")
                        {
                            const char *value;
                            dbus_message_iter_get_basic(&variant_iter, &value);
                            dev.name = value;
                        }
                        else if (std::string(prop_name) == "Address")
                        {
                            const char *value;
                            dbus_message_iter_get_basic(&variant_iter, &value);
                            dev.address = value;
                        }
                        else if (std::string(prop_name) == "Connected")
                        {
                            dbus_bool_t value;
                            dbus_message_iter_get_basic(&variant_iter, &value);
                            dev.connected = (value == TRUE);
                        }
                        else if (std::string(prop_name) == "Paired")
                        {
                            dbus_bool_t value;
                            dbus_message_iter_get_basic(&variant_iter, &value);
                            dev.paired = (value == TRUE);
                        }
                        else if (std::string(prop_name) == "Alias")
                        {
                            const char *value;
                            dbus_message_iter_get_basic(&variant_iter, &value);
                            if (dev.name.empty())
                                dev.name = value;
                        }

                        dbus_message_iter_next(&properties_iter);
                    }

                    if (dev.paired)
                    {
                        devices.push_back(dev);
                    }
                }
                dbus_message_iter_next(&interfaces_iter);
            }
            dbus_message_iter_next(&dict_iter);
        }

        dbus_message_unref(msg);
        return devices;
    }

    void BluetoothView::on_device_selected(const BluetoothDevice &device)
    {
        if (!m_initialized)
            return;
        m_selected_device = device;
        if (m_connect_button)
        {
            m_connect_button->set_enabled(true);
            m_connect_button->set_text(device.connected ? "Desconectar" : "Conectar");
        }
    }

    void BluetoothView::on_sync_new_clicked()
    {
        if (m_dialog_open)
            return;
        if (auto *app = application())
        {
            m_dialog_open = true;
            auto dialog = std::make_unique<BluetoothPairDialog>();
            dialog->when_close.connect([this](EventContext &) { m_dialog_open = false; });

            std::thread(
                [this, app, d = std::move(dialog)]() mutable
                {
                    d->initialize();
                    d->run();
                    app->post_task([this]() { m_dialog_open = false; });
                })
                .detach();
        }
    }

    void BluetoothView::on_connect_clicked()
    {
        toggle_connection();
    }

    void BluetoothView::toggle_connection()
    {
        if (m_selected_device.path.empty() || !m_dbus)
            return;

        std::string method = m_selected_device.connected ? "Disconnect" : "Connect";

        DBusMessage *msg = dbus_message_new_method_call("org.bluez", m_selected_device.path.c_str(),
                                                        "org.bluez.Device1", method.c_str());

        if (msg)
        {
            DBusError err;
            dbus_error_init(&err);
            DBusMessage *reply =
                dbus_connection_send_with_reply_and_block(m_dbus->get_connection(), msg, -1, &err);
            if (reply)
                dbus_message_unref(reply);
            if (dbus_error_is_set(&err))
            {
                std::cerr << "Bluetooth Connect/Disconnect error: " << err.message << std::endl;
                dbus_error_free(&err);
            }
            dbus_message_unref(msg);
        }

        refresh_devices();
    }
} // namespace horizon::preferences
