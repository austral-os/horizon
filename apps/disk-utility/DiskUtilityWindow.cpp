#include "DiskUtilityWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Logger.hpp>
#include "DiskInfoDialog.hpp"
#include <thread>
#include <chrono>

namespace horizon::disks
{
    DiskUtilityWindow::DiskUtilityWindow()
        : ApplicationWindow(horizon::i18n().tr("disk_utility.app_name"))
    {
        setup_toolbar();
        setup_layout();
        refresh_devices();

        signals.connect("disk.mount", [this](SignalContext&) {
            this->on_mount_requested();
        });
        signals.connect("disk.eject", [this](SignalContext&) {
            this->on_eject_requested();
        });
        signals.connect("disk.info", [this](SignalContext&) {
            this->on_info_requested();
        });

        update_toolbar_state();
    }

    void DiskUtilityWindow::setup_toolbar()
    {
        auto tb = toolbar();
        tb->set_bottom_height(58);

        auto info_btn = std::make_unique<ToolbarButton>(horizon::i18n().tr("disk_utility.toolbar.info"), "info");
        auto mount_btn = std::make_unique<ToolbarButton>(horizon::i18n().tr("disk_utility.toolbar.mount"), "media-mount");
        auto eject_btn = std::make_unique<ToolbarButton>(horizon::i18n().tr("disk_utility.toolbar.eject"), "media-eject");
        auto new_img_btn = std::make_unique<ToolbarButton>(horizon::i18n().tr("disk_utility.toolbar.new_image"), "document-new");

        auto info_ptr = info_btn.get();
        m_info_btn = info_ptr;
        info_ptr->when_click.connect([this](EventContext&) {
            this->signals.emit("disk.info");
        });
        tb->add_toolbar_widget(std::move(info_btn));
        
        auto mount_ptr = mount_btn.get();
        m_mount_btn = mount_ptr;
        mount_ptr->when_click.connect([this](EventContext&) {
            this->signals.emit("disk.mount");
        });
        tb->add_toolbar_widget(std::move(mount_btn));

        auto eject_ptr = eject_btn.get();
        m_eject_btn = eject_ptr;
        eject_ptr->when_click.connect([this](EventContext&) {
            this->signals.emit("disk.eject");
        });
        tb->add_toolbar_widget(std::move(eject_btn));
        tb->add_toolbar_widget(std::move(new_img_btn));
    }

    void DiskUtilityWindow::setup_layout()
    {
        auto main_container = std::make_unique<Widget>();
        main_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        main_container->set_position_type(WidgetPositionTypes::FILL);

        // --- Top Area (Splitted with VPanel) ---
        auto top_area = std::make_unique<VPanel>();
        top_area->set_left_width(240);
        top_area->set_position_type(WidgetPositionTypes::FILL);

        // TreeView (Left)
        auto device_tree = std::make_unique<TreeView>();
        m_device_tree = device_tree.get();
        device_tree->set_position_type(WidgetPositionTypes::FILL);
        top_area->add_child(std::move(device_tree)); // VPanel tracks children: 1st=left, 2nd=right

        // Notebook (Right)
        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();
        notebook->set_position_type(WidgetPositionTypes::FILL);

        // Tab: Borrar
        auto erase_tab = std::make_unique<EraseTab>(m_disk_manager);
        m_erase_tab = erase_tab.get();
        m_notebook->add_tab({horizon::i18n().tr("disk_utility.tabs.erase"), std::move(erase_tab)});

        auto partition_body = std::make_unique<Widget>();
        partition_body->add_child(std::make_unique<Label>(horizon::i18n().tr("disk_utility.placeholders.partitions")));
        m_notebook->add_tab({horizon::i18n().tr("disk_utility.tabs.partitions"), std::move(partition_body)});

        auto restore_body = std::make_unique<Widget>();
        restore_body->add_child(std::make_unique<Label>(horizon::i18n().tr("disk_utility.placeholders.restore")));
        m_notebook->add_tab({horizon::i18n().tr("disk_utility.tabs.restore"), std::move(restore_body)});

        top_area->add_child(std::move(notebook));

        // --- Info Panel (Bottom, Full Width) ---
        auto info_panel = std::make_unique<DiskInfoWidget>();
        info_panel->set_fixed_size(120);
        info_panel->set_border_width(1);
        info_panel->set_border_color(Color(0.85f, 0.85f, 0.85f, 1.0f));
        m_info_panel = info_panel.get();

        main_container->add_child(std::move(top_area));
        main_container->add_child(std::move(info_panel));

        set_content(std::move(main_container));

        m_device_tree->when_item_selected.connect([this](TreeViewItem *item)
                                                  { this->on_item_selected(item); });
    }

    void DiskUtilityWindow::populate_devices()
    {
        if (!m_device_tree)
            return;
        m_device_tree->clear_root_items();

        for (const auto &disk : m_disk_manager.devices())
        {
            auto disk_item =
                std::make_unique<TreeViewItem>("drive-harddisk", disk->full_model_name());

            for (const auto &part : disk->partitions)
            {
                auto part_item = std::make_unique<TreeViewItem>(
                    "partition", part->name + " (" + part->human_capacity() + ")");
                disk_item->add_child(std::move(part_item));
            }

            m_device_tree->add_root_item(std::move(disk_item));
        }
    }

    void DiskUtilityWindow::on_item_selected(TreeViewItem *item)
    {
        if (!item)
        {
            m_selected_disk = nullptr;
            m_selected_partition = nullptr;
            update_toolbar_state();
            return;
        }

        m_selected_disk = nullptr;
        m_selected_partition = nullptr;

        for (const auto &disk : m_disk_manager.devices())
        {
            if (disk->full_model_name() == item->get_text())
            {
                m_selected_disk = disk.get();
                m_info_panel->update_info(*disk);
                m_erase_tab->update_selection(m_selected_disk, nullptr);
                
                DiskItemSelectedContext ctx;
                ctx.disk = m_selected_disk;
                when_item_selected.run(ctx);
                update_toolbar_state();
                return;
            }

            for (const auto &part : disk->partitions)
            {
                if ((part->name + " (" + part->human_capacity() + ")") == item->get_text())
                {
                    m_selected_partition = part.get();
                    m_selected_disk = disk.get();
                    m_info_panel->update_info(*part);
                    m_erase_tab->update_selection(m_selected_disk, m_selected_partition);
                    
                    DiskItemSelectedContext ctx;
                    ctx.disk = disk.get();
                    ctx.partition = m_selected_partition;
                    when_item_selected.run(ctx);
                    update_toolbar_state();
                    return;
                }
            }
        }
        update_toolbar_state();
    }

    void DiskUtilityWindow::update_toolbar_state()
    {
        if (!m_mount_btn || !m_eject_btn || !m_info_btn) return;

        bool has_selection = (m_selected_disk != nullptr);
        m_info_btn->set_enabled(has_selection);

        if (!has_selection)
        {
            m_mount_btn->set_enabled(false);
            m_eject_btn->set_enabled(false);
            return;
        }

        if (m_selected_partition)
        {
            m_mount_btn->set_enabled(!m_selected_partition->is_mounted);
            m_eject_btn->set_enabled(m_selected_partition->is_mounted);
        }
        else if (m_selected_disk)
        {
            m_mount_btn->set_enabled(false);
            m_eject_btn->set_enabled(true);
        }
    }

    void DiskUtilityWindow::refresh_devices()
    {
        m_disk_manager.scan();
        populate_devices();
        update_toolbar_state();
    }

    void DiskUtilityWindow::on_mount_requested()
    {
        if (!m_selected_partition)
        {
            application()->alert(horizon::i18n().tr("disk_utility.errors.select_partition"), horizon::i18n().tr("disk_utility.toolbar.mount"));
            return;
        }

        if (m_selected_partition->is_mounted)
        {
            application()->alert("La partición ya está montada en: " + m_selected_partition->mount_point, "Montar");
            return;
        }

        auto result = m_disk_manager.mount_partition(m_selected_partition->device_path, "");
        if (result.success)
        {
            application()->alert("Petición de montaje enviada con éxito.", "Montar");
            m_disk_manager.scan();
            populate_devices();
            update_toolbar_state();
        }
        else
        {
            application()->alert("Error al montar: " + result.message, "Montar", MessageType::Error);
        }
    }

    void DiskUtilityWindow::on_eject_requested()
    {
        if (!m_selected_disk) return;

        if (m_selected_partition)
        {
            auto result = m_disk_manager.unmount_partition(m_selected_partition->device_path);
            if (result.success)
            {
                application()->alert("Partición desmontada con éxito.", "Expulsar");
                m_disk_manager.scan();
                populate_devices();
                update_toolbar_state();
            }
            else
            {
                application()->alert("Error al desmontar: " + result.message, "Expulsar", MessageType::Error);
            }
        }
    }

    void DiskUtilityWindow::on_info_requested()
    {
        if (!m_selected_disk) return;

        DiskInfo info;
        info.brand = m_selected_disk->vendor;
        info.model = m_selected_disk->model;
        info.capacity = m_selected_disk->capacity;

        for (const auto& part : m_selected_disk->partitions)
        {
            PartitionInfo p;
            p.name = part->name;
            p.mount_point = part->mount_point;
            p.capacity = part->capacity;
            p.used = part->used;
            info.partitions.push_back(p);
        }

        auto dialog = std::make_shared<DiskInfoDialog>(info);
        std::thread([dialog]() {
            dialog->initialize();
            dialog->run();
        }).detach();
    }

    void DiskUtilityWindow::initialize_monitoring()
    {
        // --- Hardware Monitoring ---
        try {
            m_monitor_helper = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
            m_monitor_helper->add_match_rule("type='signal',interface='org.freedesktop.DBus.ObjectManager',path='/org/freedesktop/UDisks2'");
        } catch (...) {
            LOG_ERROR << "[DiskUtility] Could not initialize hardware monitor";
        }

        // Safe start timer (application() must be valid here)
        if (application())
        {
            application()->add_timer(2000, [this]() {
                this->check_for_hardware_changes();
            }, true);
        }
    }

    void DiskUtilityWindow::check_for_hardware_changes()
    {
        if (!m_monitor_helper) return;

        bool changed = false;
        DBusMessage* msg;
        while ((msg = m_monitor_helper->pop_message(0)))
        {
            if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved"))
            {
                changed = true;
            }
            dbus_message_unref(msg);
        }

        if (changed)
        {
            LOG_INFO << "[DiskUtility] Hardware change detected via D-Bus, refreshing...";
            refresh_devices();
        }
    }

} // namespace horizon::disks
