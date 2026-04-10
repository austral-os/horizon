#include <algorithm>
#include <horizon/AquaObject.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <views/BluetoothView/BluetoothPairDialog.hpp>
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    BluetoothPairDialog::BluetoothPairDialog()
        : WaylandWindow("horizon.bluetooth_pair", 550, 500, true, false)
    {
        set_name(i18n().tr("preferences.bluetooth.select_device"));

        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (const std::exception &e)
        {
            std::cerr << "D-Bus init failed in BluetoothPairDialog: " << e.what() << std::endl;
        }

        setup_ui();

        // Find first adapter path
        if (m_dbus)
        {
            DBusMessage *msg = m_dbus->call_method(
                "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
            if (msg)
            {
                DBusMessageIter iter, dict_iter;
                if (dbus_message_iter_init(msg, &iter))
                {
                    dbus_message_iter_recurse(&iter, &dict_iter);
                    while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY)
                    {
                        DBusMessageIter entry_iter, interfaces_iter;
                        dbus_message_iter_recurse(&dict_iter, &entry_iter);
                        const char *object_path;
                        dbus_message_iter_get_basic(&entry_iter, &object_path);
                        dbus_message_iter_next(&entry_iter);
                        dbus_message_iter_recurse(&entry_iter, &interfaces_iter);
                        while (dbus_message_iter_get_arg_type(&interfaces_iter) ==
                               DBUS_TYPE_DICT_ENTRY)
                        {
                            DBusMessageIter interface_entry;
                            dbus_message_iter_recurse(&interfaces_iter, &interface_entry);
                            const char *interface_name;
                            dbus_message_iter_get_basic(&interface_entry, &interface_name);
                            if (std::string(interface_name) == "org.bluez.Adapter1")
                            {
                                m_adapter_path = object_path;
                                break;
                            }
                            dbus_message_iter_next(&interfaces_iter);
                        }
                        dbus_message_iter_next(&dict_iter);
                    }
                }
                dbus_message_unref(msg);
            }
        }

        start_scanning();
    }

    BluetoothPairDialog::~BluetoothPairDialog()
    {
        stop_scanning();
    }

    void BluetoothPairDialog::setup_ui()
    {
        auto root_wnd = std::make_unique<Window>(i18n().tr("preferences.bluetooth.select_device"));
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_margin(20);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(15);

        // 1. Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.bluetooth.select_device"));
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_font_size(18);
        title->set_fixed_size(35);
        container->add_child(std::move(title));

        // 2. Search Box
        auto search_box = std::make_unique<TextBox<TextPolicy>>();
        search_box->set_placeholder(i18n().tr("preferences.bluetooth.search"));
        search_box->set_fixed_size(35);
        search_box->when_text_changed.connect([this](KeyEventContext &)
                                              { this->filter_devices(m_search_box->text()); });
        m_search_box = search_box.get();
        container->add_child(std::move(search_box));

        // 3. Table View
        auto table = std::make_unique<TableView<BluetoothDevice>>();
        table->set_header_visible(false);

        TableColumn<BluetoothDevice> icon_col;
        icon_col.width = 35;
        icon_col.cell_factory = [](const BluetoothDevice &)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("bluetooth-active");
            icon->set_icon_size(20);
            return icon;
        };
        table->add_column(std::move(icon_col));

        TableColumn<BluetoothDevice> name_col;
        name_col.width = 415;
        name_col.cell_factory = [](const BluetoothDevice &data)
        {
            auto lbl = std::make_unique<Label>(data.name.empty() ? data.address : data.name);
            return lbl;
        };
        table->add_column(std::move(name_col));

        m_table_view = table.get();
        m_table_view->when_row_click.connect([this](auto &ctx)
                                             { this->on_device_selected(ctx.row_data); });
        container->add_child(std::move(table));

        // 4. Bottom Status Area (LoadingBar + PIN info)
        auto bottom_status = std::make_unique<Widget>();
        bottom_status->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_status->set_spacing(10);
        bottom_status->set_fixed_size(35);

        auto refresh_icon = std::make_unique<Icon>();
        refresh_icon->set_icon_name("view-refresh");
        refresh_icon->set_fixed_size(20); // Simulating refresh spin
        bottom_status->add_child(std::move(refresh_icon));

        auto scan_label = std::make_unique<Label>(i18n().tr("preferences.bluetooth.scanning"));
        m_status_label = scan_label.get();
        bottom_status->add_child(std::move(scan_label));

        bottom_status->add_child(Spacer(20));

        // Manual PIN (placeholder for now)
        auto pin_label = std::make_unique<Label>(i18n().tr("preferences.bluetooth.pin_manual"));
        pin_label->set_alignment(TextAlignment::Right);
        bottom_status->add_child(std::move(pin_label));

        auto pin_input = std::make_unique<TextBox<TextPolicy>>();
        pin_input->set_placeholder("0000");
        pin_input->set_fixed_size(80);
        bottom_status->add_child(std::move(pin_input));

        container->add_child(std::move(bottom_status));

        // Loading Bar
        auto loading_bar = std::make_unique<LoadingBar>();
        loading_bar->set_fixed_size(25); // Thin line at bottom
        m_loading_bar = loading_bar.get();
        container->add_child(std::move(loading_bar));

        // 5. Buttons
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text(i18n().tr("preferences.common.cancel"));
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        m_cancel_btn = btn_cancel.get();
        buttons->add_child(std::move(btn_cancel));

        auto btn_next = std::make_unique<Button<AquaObject>>();
        btn_next->set_text(i18n().tr("preferences.bluetooth.next"));
        btn_next->set_fixed_size(100);
        btn_next->set_accent_color(WidgetAccentColor::Primary);
        btn_next->set_enabled(false);
        btn_next->when_click.connect([this](MouseButtonEventContext &)
                                     { this->on_next_clicked(); });
        m_next_btn = btn_next.get();
        buttons->add_child(std::move(btn_next));

        container->add_child(std::move(buttons));
        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void BluetoothPairDialog::start_scanning()
    {
        if (!m_dbus)
            return;
        m_dbus->call_method_void("org.bluez", m_adapter_path, "org.bluez.Adapter1",
                                 "StartDiscovery");

        m_stop_scan = false;
        m_scan_thread = std::thread(&BluetoothPairDialog::monitor_discovery, this);

        if (m_loading_bar)
            m_loading_bar->set_visible(true);
    }

    void BluetoothPairDialog::stop_scanning()
    {
        m_stop_scan = true;
        if (m_scan_thread.joinable())
            m_scan_thread.join();

        if (m_dbus)
        {
            m_dbus->call_method_void("org.bluez", m_adapter_path, "org.bluez.Adapter1",
                                     "StopDiscovery");
        }
    }

    void BluetoothPairDialog::monitor_discovery()
    {
        std::cerr << "BluetoothPairDialog: Monitoring discovery on adapter " << m_adapter_path
                  << std::endl;
        while (!m_stop_scan)
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (!m_dbus)
                continue;

            DBusMessage *msg = m_dbus->call_method(
                "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
            if (!msg)
            {
                std::cerr << "BluetoothPairDialog: GetManagedObjects failed" << std::endl;
                continue;
            }

            std::vector<BluetoothDevice> new_list;
            DBusMessageIter iter, dict_iter;
            if (dbus_message_iter_init(msg, &iter))
            {
                dbus_message_iter_recurse(&iter, &dict_iter);
                while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY)
                {
                    DBusMessageIter entry_iter, interfaces_iter;
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

                            while (dbus_message_iter_get_arg_type(&properties_iter) ==
                                   DBUS_TYPE_DICT_ENTRY)
                            {
                                DBusMessageIter prop_entry, variant_iter;
                                dbus_message_iter_recurse(&properties_iter, &prop_entry);
                                const char *prop_name;
                                dbus_message_iter_get_basic(&prop_entry, &prop_name);
                                dbus_message_iter_next(&prop_entry);
                                dbus_message_iter_recurse(&prop_entry, &variant_iter);

                                if (std::string(prop_name) == "Name")
                                {
                                    if (dbus_message_iter_get_arg_type(&variant_iter) ==
                                        DBUS_TYPE_STRING)
                                    {
                                        const char *value;
                                        dbus_message_iter_get_basic(&variant_iter, &value);
                                        dev.name = value;
                                    }
                                }
                                else if (std::string(prop_name) == "Address")
                                {
                                    if (dbus_message_iter_get_arg_type(&variant_iter) ==
                                        DBUS_TYPE_STRING)
                                    {
                                        const char *value;
                                        dbus_message_iter_get_basic(&variant_iter, &value);
                                        dev.address = value;
                                    }
                                }
                                else if (std::string(prop_name) == "Paired")
                                {
                                    if (dbus_message_iter_get_arg_type(&variant_iter) ==
                                        DBUS_TYPE_BOOLEAN)
                                    {
                                        dbus_bool_t value;
                                        dbus_message_iter_get_basic(&variant_iter, &value);
                                        dev.paired = (value == TRUE);
                                    }
                                }
                                else if (std::string(prop_name) == "Alias")
                                {
                                    if (dbus_message_iter_get_arg_type(&variant_iter) ==
                                        DBUS_TYPE_STRING)
                                    {
                                        const char *value;
                                        dbus_message_iter_get_basic(&variant_iter, &value);
                                        if (dev.name.empty())
                                            dev.name = value;
                                    }
                                }
                                dbus_message_iter_next(&properties_iter);
                            }
                            if (!dev.paired)
                            {
                                std::cerr << "BluetoothPairDialog: Discovered device " << dev.name
                                          << " [" << dev.address << "]" << std::endl;
                                new_list.push_back(dev);
                            }
                        }
                        dbus_message_iter_next(&interfaces_iter);
                    }
                    dbus_message_iter_next(&dict_iter);
                }
            }
            dbus_message_unref(msg);

            this->post_task(
                [this, new_list]()
                {
                    m_discovered_devices = new_list;
                    this->filter_devices(m_search_box ? m_search_box->text() : "");
                });
        }
    }

    void BluetoothPairDialog::filter_devices(const std::string &query)
    {
        if (query.empty())
        {
            m_table_view->set_data(m_discovered_devices);
            return;
        }

        std::vector<BluetoothDevice> filtered;
        for (const auto &d : m_discovered_devices)
        {
            std::string name = d.name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::string q = query;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);

            if (name.find(q) != std::string::npos)
            {
                filtered.push_back(d);
            }
        }
        m_table_view->set_data(filtered);
    }

    void BluetoothPairDialog::on_device_selected(const BluetoothDevice &device)
    {
        m_selected_device = device;
        if (m_next_btn)
            m_next_btn->set_enabled(true);
    }

    void BluetoothPairDialog::on_next_clicked()
    {
        if (m_selected_device.path.empty() || !m_dbus)
            return;

        if (m_status_label)
            m_status_label->set_text(i18n().tr("preferences.bluetooth.pairing"));
        if (m_next_btn)
            m_next_btn->set_enabled(false);

        std::thread(
            [this]()
            {
                DBusMessage *msg = dbus_message_new_method_call(
                    "org.bluez", m_selected_device.path.c_str(), "org.bluez.Device1", "Pair");

                if (msg)
                {
                    DBusError err;
                    dbus_error_init(&err);
                    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
                        m_dbus->get_connection(), msg, -1, &err);

                    if (reply)
                        dbus_message_unref(reply);
                    if (dbus_error_is_set(&err))
                        dbus_error_free(&err);
                    dbus_message_unref(msg);

                    this->post_task(
                        [this, success = (reply != nullptr),
                         err_msg = (dbus_error_is_set(&err) ? std::string(err.message) : "")]()
                        {
                            if (success)
                            {
                                if (m_status_label)
                                    m_status_label->set_text(i18n().tr("preferences.bluetooth.paired_success"));
                                this->quit();
                            }
                            else
                            {
                                if (m_status_label)
                                    m_status_label->set_text(i18n().tr("preferences.common.error_details", {{"0", err_msg}}));
                                if (m_next_btn)
                                    m_next_btn->set_enabled(true);
                            }
                        });
                }
            })
            .detach();
    }
} // namespace horizon::preferences
