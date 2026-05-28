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
#include <horizon/storage/RemoteManagerBase.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace
{
    std::string uri_decode(const std::string& encoded) {
        std::string res;
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%') {
                if (i + 2 < encoded.length()) {
                    int value;
                    std::istringstream is(encoded.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        res += static_cast<char>(value);
                        i += 2;
                    }
                }
            } else {
                res += encoded[i];
            }
        }
        return res;
    }
}

namespace horizon::files
{
    class DiskSidebarItem : public horizon::SidebarItem
    {
    public:
        DiskSidebarItem(const std::string &icon_name, const std::string &text,
                        disks::DiskPartition *part, disks::DiskManager *manager, FileSidebar *parent_sidebar)
            : SidebarItem(icon_name, text), m_partition(part), m_manager(manager), m_parent_sidebar(parent_sidebar)
        {
            if (m_partition->is_mounted)
            {
                add_child(horizon::Spacer());

                auto icon = std::make_unique<Icon>();
                icon->set_icon_name("media-eject-symbolic");
                icon->set_icon_size(16);
                icon->set_fixed_size(24);

                icon->when_click.connect(
                    [this](MouseButtonEventContext &ctx)
                    {
                        ctx.stop_propagation = true;
                        if (m_manager)
                        {
                            if (application() && application()->w_surface())
                                application()->w_surface()->set_cursor(CursorType::Wait);

                            LOG_INFO << "DiskSidebarItem: Desmontando " << m_partition->device_path;
                            std::string old_path = m_partition->mount_point;
                            auto *sidebar = m_parent_sidebar; // Capture locally
                            
                            m_manager->unmount_partition(m_partition->device_path);
                            
                            if (!old_path.empty() && sidebar)
                            {
                                sidebar->refresh_devices(); // This deletes 'this'!
                                UnmountEventContext unmount_ctx;
                                unmount_ctx.mount_path = old_path;
                                sidebar->when_resource_unmounted.run(unmount_ctx);
                            }

                            if (application() && application()->w_surface())
                                application()->w_surface()->set_cursor(CursorType::Default);
                        }
                    });

                add_child(std::move(icon));
            }
        }

    private:
        disks::DiskPartition *m_partition;
        disks::DiskManager *m_manager;
        FileSidebar *m_parent_sidebar;
    };

    class RemoteSidebarItem : public horizon::SidebarItem
    {
    public:
        RemoteSidebarItem(const std::string &icon_name, const std::string &text,
                          const std::string &uri, FileSidebar *parent_sidebar)
            : SidebarItem(icon_name, text), m_uri(uri), m_parent_sidebar(parent_sidebar)
        {
            if (m_label_ptr)
            {
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
                    FileSidebar *sidebar = m_parent_sidebar;
                    LOG_INFO << "RemoteSidebarItem: Intentando desmontar " << uri;

                    if (sidebar && sidebar->remote_storage())
                    {
                        if (sidebar->application() && sidebar->application()->w_surface())
                            sidebar->application()->w_surface()->set_cursor(CursorType::Wait);

                        // Buscar la ruta de montaje antes de que se pierda
                        std::string mount_path;
                        auto mounts = sidebar->remote_storage()->get_active_mounts();
                        for (auto &m : mounts)
                        {
                            if (m.uri == uri)
                            {
                                mount_path = m.mount_path;
                                break;
                            }
                        }

                        sidebar->remote_storage()->when_unmount_by_uri(
                            uri,
                            [sidebar, mount_path](bool success, std::string msg)
                            {
                                if (success)
                                {
                                    LOG_INFO << "RemoteSidebarItem: Desmontado exitoso. Esperando "
                                                "para refrescar...";
                                    
                                    if (sidebar)
                                    {
                                        sidebar->refresh_devices(); // This deletes the item!
                                        
                                        if (!mount_path.empty())
                                        {
                                            UnmountEventContext ctx;
                                            ctx.mount_path = mount_path;
                                            sidebar->when_resource_unmounted.run(ctx);
                                        }
                                    }
                                }
                                else
                                {
                                    LOG_ERROR << "RemoteSidebarItem: Error al desmontar: " << msg;
                                }

                                if (sidebar && sidebar->application() && sidebar->application()->w_surface())
                                    sidebar->application()->w_surface()->set_cursor(
                                        CursorType::Default);
                            });
                    }
                });

            add_child(std::move(icon));
        }

    private:
        std::string m_uri;
        FileSidebar *m_parent_sidebar;
    };

    FileSidebar::FileSidebar() : Sidebar()
    {

        when_application_load.connect(
            [this](EventContext &)
            {
                this->setup_monitoring();
                if (application())
                {
                    application()->add_timer(2000, [this]() { this->refresh_devices(); }, false);
                }
            });

        refresh_devices();

        m_disk_manager.when_hardware_changed.connect([this](disks::HardwareChangedContext &)
                                                     { this->refresh_devices(); });

        // m_remote_manager will be set by the application if needed
    }

    void FileSidebar::setup_monitoring()
    {
        if (application())
        {
            application()->add_timer(
                2000, [this]() { this->m_disk_manager.check_hardware_changes(); }, true);
        }
    }

    void FileSidebar::set_remote_storage(storage::RemoteManagerBase *manager)
    {
        m_remote_manager = manager;
        if (m_remote_manager)
        {
            m_remote_manager->when_changed.connect(
                [this](storage::RemoteStorageEventContext &)
                {
                    if (application())
                    {
                        application()->post_task([this]() { this->refresh_devices(); });
                    }
                });
        }
        refresh_devices();
    }

    void FileSidebar::refresh_devices()
    {
        std::string last_path;
        if (m_selected_item) last_path = m_selected_item->path();

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

        auto item_trash = std::make_unique<horizon::SidebarItem>("user-trash", "Papelera");
        std::string trash_path = std::string(home) + "/.local/share/Trash/files";
        item_trash->set_path(trash_path);
        add_item("Favorites", std::move(item_trash));

        // --- Add Bookmarks ---
        std::string bookmarks_path = std::string(home) + "/.config/gtk-3.0/bookmarks";
        if (std::filesystem::exists(bookmarks_path))
        {
            std::ifstream file(bookmarks_path);
            if (file.is_open())
            {
                std::string line;
                bool has_bookmarks = false;
                while (std::getline(file, line))
                {
                    if (line.empty()) continue;
                    
                    std::string uri, name;
                    std::istringstream iss(line);
                    iss >> uri;
                    if (iss >> std::ws)
                    {
                        std::getline(iss, name);
                    }
                    
                    if (uri.find("file://") == 0)
                    {
                        if (!has_bookmarks)
                        {
                            add_group("Marcadores");
                            has_bookmarks = true;
                        }
                        
                        std::string path = uri_decode(uri.substr(7)); // remove "file://"
                        if (name.empty()) {
                            name = std::filesystem::path(path).filename().string();
                        }
                        auto item = std::make_unique<horizon::SidebarItem>("folder", name);
                        item->set_path(path);
                        
                        item->when_right_click.connect([this, uri, bookmarks_path](horizon::MouseButtonEventContext& ctx) {
                            ctx.stop_propagation = true;
                            m_context_menu = std::make_unique<horizon::Menu>();
                            auto r_item = m_context_menu->add_item("Eliminar de marcadores");
                            r_item->when_click.connect([this, uri, bookmarks_path](auto&) {
                                application()->post_task([this, uri, bookmarks_path]() {
                                    std::vector<std::string> lines;
                                    std::ifstream in(bookmarks_path);
                                    if (in.is_open()) {
                                        std::string line;
                                        while (std::getline(in, line)) {
                                            if (line.empty()) continue;
                                            std::istringstream iss(line);
                                            std::string current_uri;
                                            iss >> current_uri;
                                            if (current_uri != uri) {
                                                lines.push_back(line);
                                            }
                                        }
                                        in.close();
                                    }
                                    std::ofstream out(bookmarks_path);
                                    if (out.is_open()) {
                                        for (const auto& l : lines) {
                                            out << l << "\n";
                                        }
                                        out.close();
                                    }
                                    refresh_devices();
                                });
                            });
                            application()->show_context_menu(m_context_menu.get(), -1, -1, ctx.serial, this);
                        });
                        
                        add_item("Marcadores", std::move(item));
                    }
                }
            }
        }

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
                        std::make_unique<DiskSidebarItem>(icon, label, part.get(), &m_disk_manager, this);

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
        if (m_remote_manager)
        {
            auto remote_mounts = m_remote_manager->get_active_mounts();
            bool has_network = false;

            for (const auto &mount : remote_mounts)
            {
                if (!has_network)
                {
                    add_group("Network");
                    has_network = true;
                }

                auto item = std::make_unique<RemoteSidebarItem>(mount.icon_name, mount.name,
                                                                mount.uri, this);
                item->set_path(mount.mount_path);
                add_item("Network", std::move(item));
            }
        }

        if (!last_path.empty()) select_item_by_path(last_path);
    }
} // namespace horizon::files
