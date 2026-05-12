#include "horizon/files/FileSidebar.hpp"
#include <horizon/XdgUserDirs.hpp>
#include <horizon/SidebarItem.hpp>
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <gio/gio.h>
#include <cstdlib>
#include <thread>

namespace horizon::files
{
    class DiskSidebarItem : public horizon::SidebarItem
    {
    public:
        DiskSidebarItem(const std::string &icon_name, const std::string &text, disks::DiskPartition *part, disks::DiskManager *manager)
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

                eject_btn->when_mouse_enter.connect([btn = eject_btn.get()](EventContext&) {
                    btn->set_background_color(Color(1.0f, 1.0f, 1.0f, 0.2f));
                });
                eject_btn->when_mouse_leave.connect([btn = eject_btn.get()](EventContext&) {
                    btn->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
                });

                eject_btn->when_click.connect([this](MouseButtonEventContext &ctx) {
                    ctx.stop_propagation = true;
                    if (m_manager) {
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
        RemoteSidebarItem(const std::string &icon_name, const std::string &text, const std::string &mount_path)
            : SidebarItem(icon_name, text), m_mount_path(mount_path)
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

            eject_btn->when_mouse_enter.connect([btn = eject_btn.get()](EventContext&) {
                btn->set_background_color(Color(1.0f, 1.0f, 1.0f, 0.2f));
            });
            eject_btn->when_mouse_leave.connect([btn = eject_btn.get()](EventContext&) {
                btn->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
            });

            eject_btn->when_click.connect([this](MouseButtonEventContext &ctx) {
                ctx.stop_propagation = true;
                
                GFile* file = g_file_new_for_path(m_mount_path.c_str());
                GMount* mount = g_file_find_enclosing_mount(file, nullptr, nullptr);
                if (mount) {
                    g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr, nullptr, nullptr);
                    g_object_unref(mount);
                }
                g_object_unref(file);
            });
            add_child(std::move(eject_btn));
        }

    private:
        std::string m_mount_path;
    };

    FileSidebar::FileSidebar() : Sidebar()
    {
        when_application_load.connect([this](EventContext&) {
            this->setup_monitoring();
        });

        refresh_devices();

        m_disk_manager.when_hardware_changed.connect([this](disks::HardwareChangedContext&) {
            this->refresh_devices();
        });

        GVolumeMonitor* monitor = g_volume_monitor_get();
        g_signal_connect_swapped(monitor, "mount-added", G_CALLBACK(+[](FileSidebar* self) {
            if (self->application()) self->application()->post_task([self]() { self->refresh_devices(); });
        }), this);
        g_signal_connect_swapped(monitor, "mount-removed", G_CALLBACK(+[](FileSidebar* self) {
            if (self->application()) self->application()->post_task([self]() { self->refresh_devices(); });
        }), this);
    }

    void FileSidebar::setup_monitoring()
    {
        if (application())
        {
            application()->add_timer(2000, [this]() {
                this->m_disk_manager.check_hardware_changes();
            }, true);
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

        auto item_downloads = std::make_unique<horizon::SidebarItem>("folder-download", "Downloads");
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
                    std::string icon = disk->is_removable ? "drive-removable-media" : "drive-harddisk";
                    
                    auto item = std::make_unique<DiskSidebarItem>(icon, label, part.get(), &m_disk_manager);
                    
                    if (part->is_mounted) {
                        item->set_path(part->mount_point);
                    } else {
                        std::string dev_path = part->device_path;
                        item->when_click.connect([this, dev_path](auto&) {
                            auto result = m_disk_manager.mount_partition(dev_path, "");
                            if (result.success) {
                                std::thread([this]() {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                    if (application()) {
                                        application()->post_task([this]() { this->refresh_devices(); });
                                    }
                                }).detach();
                            }
                        });
                    }
                    add_item("Devices", std::move(item));
                }
            }
        }

        // --- Add Remote Mounts using GIO directly ---
        GVolumeMonitor* monitor = g_volume_monitor_get();
        GList* g_mounts = g_volume_monitor_get_mounts(monitor);
        bool has_network = false;

        for (GList* l = g_mounts; l != nullptr; l = l->next) {
            GMount* mount = G_MOUNT(l->data);
            GFile* root = g_mount_get_root(mount);
            char* uri = g_file_get_uri(root);
            
            std::string s_uri = uri ? uri : "";
            if (s_uri.find("://") != std::string::npos && s_uri.find("file://") != 0) {
                if (!has_network) {
                    add_group("Network");
                    has_network = true;
                }

                char* name = g_mount_get_name(mount);
                char* path = g_file_get_path(root);
                GIcon* icon = g_mount_get_icon(mount);
                std::string icon_name = "folder-remote";
                if (G_IS_THEMED_ICON(icon)) {
                    const char* const* names = g_themed_icon_get_names(G_THEMED_ICON(icon));
                    if (names && names[0]) icon_name = names[0];
                }

                auto item = std::make_unique<RemoteSidebarItem>(icon_name, name ? name : "Remote", path ? path : "");
                item->set_path(path ? path : "");
                add_item("Network", std::move(item));

                if (name) g_free(name);
                if (path) g_free(path);
                if (icon) g_object_unref(icon);
            }
            if (uri) g_free(uri);
            g_object_unref(root);
        }
        g_list_free_full(g_mounts, g_object_unref);
        g_object_unref(monitor);
    }
} // namespace horizon::files
