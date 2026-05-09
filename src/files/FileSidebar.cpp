#include "horizon/files/FileSidebar.hpp"
#include <horizon/XdgUserDirs.hpp>
#include <horizon/SidebarItem.hpp>
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <cstdlib>

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
                // Add a spacer to push the eject button to the right
                add_child(horizon::Spacer());

                auto eject_btn = std::make_unique<Button<Widget>>();
                eject_btn->set_fixed_size(24);
                eject_btn->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
                eject_btn->set_border_radius(4);
                
                auto icon = std::make_unique<Icon>();
                icon->set_icon_name("media-eject");
                icon->set_icon_size(16);
                icon->set_position_type(WidgetPositionTypes::FREE);
                icon->set_position(4, 4); // Centered in a 24x24 button
                eject_btn->add_child(std::move(icon));

                // Add hover effect manually since Button<Widget> doesn't have it built-in like AquaObject
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
                
                // Extra margin to the right
                auto end_spacer = std::make_unique<Widget>();
                end_spacer->set_fixed_size(4);
                add_child(std::move(end_spacer));
            }
        }

    private:
        disks::DiskPartition *m_partition;
        disks::DiskManager *m_manager;
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

        when_item_selected.connect([this](SidebarItemSelectedContext& ctx) {
            // Check if it's a disk item by checking path or some other way.
            // For now, if it's not mounted and we click it, we should mount it.
            // But SidebarItem doesn't know about its "mounted" status easily here.
            // We can handle the click inside refresh_devices by connecting to the item's own click.
        });
    }

    void FileSidebar::setup_monitoring()
    {
        // Start a timer to check for hardware changes
        if (application())
        {
            LOG_INFO << "[FileSidebar] Starting hardware monitoring timer";
            application()->add_timer(2000, [this]() {
                this->m_disk_manager.check_hardware_changes();
            }, true);
        }
        else {
            LOG_ERROR << "[FileSidebar] setup_monitoring failed: application() is null";
        }
    }

    void FileSidebar::refresh_devices()
    {
        LOG_INFO << "[FileSidebar] Refreshing devices list";
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

        // --- Add Devices ---
        m_disk_manager.scan();
        bool has_devices = false;

        for (const auto &disk : m_disk_manager.devices())
        {
            for (const auto &part : disk->partitions)
            {
                // Only show partitions that are likely user data (or removable)
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
                        // If not mounted, we set the device path and handle the click
                        std::string dev_path = part->device_path;
                        item->when_click.connect([this, dev_path](auto&) {
                            auto result = m_disk_manager.mount_partition(dev_path, "");
                            if (result.success) {
                                // Wait a bit and refresh
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
    }
} // namespace horizon::files
