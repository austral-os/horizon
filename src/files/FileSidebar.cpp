#include "horizon/files/FileSidebar.hpp"
#include <cstdlib>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <horizon/SidebarItem.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/XdgUserDirs.hpp>
#include <horizon/storage/RemoteManager.hpp>

namespace horizon::files
{
    class DiskSidebarItem : public horizon::SidebarItem
    {
    public:
        DiskSidebarItem(const std::string &icon_name, const std::string &text,
                        disks::DiskPartition *part, disks::DiskManager *manager)
            : SidebarItem(icon_name, text), m_partition(part), m_manager(manager)
        {
            if (m_partition->is_mounted)
            {
                add_child(horizon::Spacer());

                auto eject_btn = std::make_unique<Button<Widget>>();
                eject_btn->set_fixed_size(24);
                eject_btn->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
                eject_btn->set_border_radius(4);

                auto icon = std::make_unique<Icon>();
                icon->set_icon_name("media-eject");
                icon->set_icon_size(16);
                icon->set_position_type(WidgetPositionTypes::FREE);
                icon->set_position(4, 4);
                eject_btn->add_child(std::move(icon));

                eject_btn->when_mouse_enter.connect(
                    [btn = eject_btn.get()](EventContext &)
                    { btn->set_background_color(Color(1.0f, 1.0f, 1.0f, 0.2f)); });
                eject_btn->when_mouse_leave.connect(
                    [btn = eject_btn.get()](EventContext &)
                    { btn->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f)); });

                eject_btn->when_click.connect(
                    [this](MouseButtonEventContext &ctx)
                    {
                        ctx.stop_propagation = true;
                        if (m_manager)
                        {
                            m_manager->unmount_partition(m_partition->device_path);
                        }
                    });
                add_child(std::move(eject_btn));

                auto end_spacer = std::make_unique<Widget>();
                end_spacer->set_fixed_size(4);
                add_child(std::move(end_spacer));
            }
        }

    private:
        disks::DiskPartition *m_partition;
        disks::DiskManager *m_manager;
    };

    class RemoteSidebarItem : public horizon::SidebarItem
    {
    public:
        RemoteSidebarItem(const std::string &icon_name, const std::string &text,
                          const std::string &uri, FileSidebar* parent_sidebar)
            : SidebarItem(icon_name, text), m_uri(uri), m_parent_sidebar(parent_sidebar)
        {
            if (m_label_ptr) {
                m_label_ptr->set_fixed_size(-1);
            }

            add_child(horizon::Spacer());

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("media-eject-symbolic");
            icon->set_icon_size(16);
            icon->set_fixed_size(24);
            
            icon->when_click.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    ctx.stop_propagation = true;
                    std::string uri = m_uri;
                    FileSidebar* sidebar = m_parent_sidebar;
                    LOG_INFO << "RemoteSidebarItem: Intentando desmontar " << uri;
                    
                    if (sidebar && sidebar->remote_storage()) {
                        // Buscar la ruta de montaje antes de que se pierda
                        std::string mount_path;
                        auto mounts = sidebar->remote_storage()->get_active_mounts();
                        for (auto &m : mounts) {
                            if (m.uri == uri) {
                                mount_path = m.mount_path;
                                break;
                            }
                        }

                        sidebar->remote_storage()->when_unmount_by_uri(uri, [sidebar, mount_path](bool success, std::string msg) {
                            if (success) {
                                LOG_INFO << "RemoteSidebarItem: Desmontado exitoso. Esperando para refrescar...";
                                
                                if (!mount_path.empty()) {
                                    UnmountEventContext ctx;
                                    ctx.mount_path = mount_path;
                                    sidebar->when_resource_unmounted.run(ctx);
                                }

                                if (sidebar->application()) {
                                    sidebar->application()->add_timer(200, [sidebar]() {
                                        sidebar->refresh_devices();
                                    }, false);
                                }
                            } else {
                                LOG_ERROR << "RemoteSidebarItem: Error al desmontar: " << msg;
                            }
                        });
                    }
                });

            add_child(std::move(icon));
            
            auto padding = std::make_unique<Widget>();
            padding->set_fixed_size(10);
            add_child(std::move(padding));
        }

    private:
        std::string m_uri;
        FileSidebar* m_parent_sidebar;
    };

    FileSidebar::FileSidebar() : Sidebar()
    {
        m_remote_manager = std::make_unique<storage::RemoteManager>();
        
        when_application_load.connect([this](EventContext &) { this->setup_monitoring(); });

        refresh_devices();

        m_disk_manager.when_hardware_changed.connect([this](disks::HardwareChangedContext &)
                                                     { this->refresh_devices(); });

        m_remote_manager->when_changed.connect([this](storage::RemoteStorageEventContext &) {
            if (application()) {
                application()->post_task([this]() { this->refresh_devices(); });
            }
        });
    }

    void FileSidebar::setup_monitoring()
    {
        if (application())
        {
            application()->add_timer(
                2000, [this]() { this->m_disk_manager.check_hardware_changes(); }, true);
        }
    }

    void FileSidebar::refresh_devices()
    {
        clear();
        add_group("Favorites");

        auto home = getenv("HOME") ? getenv("HOME") : "/home/user";

        auto item_home = std::make_unique<horizon::SidebarItem>("user-home", "All My Files");
        item_home->set_path(home);
        add_item("Favorites", std::move(item_home));

        auto item_apps = std::make_unique<horizon::SidebarItem>("system-run", "Aplicaciones");
        item_apps->set_path("/usr/share/applications");
        add_item("Favorites", std::move(item_apps));

        auto item_desktop = std::make_unique<horizon::SidebarItem>("user-desktop", "Desktop");
        item_desktop->set_path(horizon::XdgUserDirs::get_desktop());
        add_item("Favorites", std::move(item_desktop));

        auto item_docs = std::make_unique<horizon::SidebarItem>("folder-documents", "Documents");
        item_docs->set_path(horizon::XdgUserDirs::get_documents());
        add_item("Favorites", std::move(item_docs));

        auto item_downloads =
            std::make_unique<horizon::SidebarItem>("folder-download", "Downloads");
        item_downloads->set_path(horizon::XdgUserDirs::get_download());
        add_item("Favorites", std::move(item_downloads));

        m_disk_manager.scan();
        bool has_devices = false;

        for (const auto &disk : m_disk_manager.devices())
        {
            for (const auto &part : disk->partitions)
            {
                if (disk->is_removable || part->is_mounted)
                {
                    if (!has_devices)
                    {
                        add_group("Devices");
                        has_devices = true;
                    }

                    std::string label = part->label.empty() ? part->name : part->label;
                    std::string icon =
                        disk->is_removable ? "drive-removable-media" : "drive-harddisk";

                    auto item =
                        std::make_unique<DiskSidebarItem>(icon, label, part.get(), &m_disk_manager);

                    if (part->is_mounted)
                    {
                        item->set_path(part->mount_point);
                    }
                    else
                    {
                        std::string dev_path = part->device_path;
                        item->when_click.connect(
                            [this, dev_path](auto &)
                            {
                                auto result = m_disk_manager.mount_partition(dev_path, "");
                                if (result.success)
                                {
                                    std::thread(
                                        [this]()
                                        {
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(500));
                                            if (application())
                                            {
                                                application()->post_task(
                                                    [this]() { this->refresh_devices(); });
                                            }
                                        })
                                        .detach();
                                }
                            });
                    }
                    add_item("Devices", std::move(item));
                }
            }
        }

        // --- Add Remote Mounts using RemoteManager abstraction ---
        auto remote_mounts = m_remote_manager->get_active_mounts();
        bool has_network = false;

        for (const auto &mount : remote_mounts)
        {
            if (!has_network)
            {
                add_group("Network");
                has_network = true;
            }

            auto item = std::make_unique<RemoteSidebarItem>(mount.icon_name, mount.name, mount.uri, this);
            item->set_path(mount.mount_path);
            add_item("Network", std::move(item));
        }
    }
} // namespace horizon::files
