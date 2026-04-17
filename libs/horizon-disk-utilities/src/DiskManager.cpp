#include "horizon-disk-utilities/DiskManager.hpp"
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/statvfs.h>
#include <libmount/libmount.h>
#include <iostream>

namespace horizon::disks
{
    static std::string read_file(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        // Trim trailing whitespace and newlines
        content.erase(content.find_last_not_of(" \n\r\t") + 1);
        return content;
    }

    DiskManager::DiskManager()
    {
        try {
            m_dbus_helper = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        } catch (...) {
            std::cerr << "[DiskManager] Warning: Could not initialize DBusHelper" << std::endl;
        }
    }

    DiskManager::~DiskManager() = default;

    void DiskManager::scan()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_devices.clear();
        scan_sys_block();
        scan_mounts();
    }

    void DiskManager::scan_sys_block()
    {
        namespace fs = std::filesystem;
        if (!fs::exists("/sys/block")) return;

        for (const auto &entry : fs::directory_iterator("/sys/block"))
        {
            std::string name = entry.path().filename().string();
            
            // Skip loop devices and ram disks
            if (name.find("loop") == 0 || name.find("ram") == 0) continue;

            auto device = std::make_unique<DiskDevice>();
            device->name = name;
            device->device_path = "/dev/" + name;
            device->udisks_path = "/org/freedesktop/UDisks2/block_devices/" + name;
            
            std::string sys_path = entry.path().string();
            
            device->model = read_file(sys_path + "/device/model");
            device->vendor = read_file(sys_path + "/device/vendor");
            
            if (device->model.empty()) device->model = "Unknown Disk";
            if (device->vendor.empty()) device->vendor = "Generic";

            std::string size_str = read_file(sys_path + "/size");
            if (!size_str.empty())
            {
                try {
                    device->capacity = std::stoull(size_str) * 512;
                } catch (...) {
                    device->capacity = 0;
                }
            }

            std::string removable = read_file(sys_path + "/removable");
            device->is_removable = (removable == "1");

            std::string rotational = read_file(sys_path + "/queue/rotational");
            device->is_ssd = (rotational == "0");

            // Look for partitions
            for (const auto &sub_entry : fs::directory_iterator(sys_path))
            {
                std::string sub_name = sub_entry.path().filename().string();
                // Partitions usually start with the disk name followed by a number (e.g., sda1)
                // or pN for nvme (e.g., nvme0n1p1)
                if (sub_name.find(name) == 0 && sub_name != name)
                {
                    auto partition = std::make_unique<DiskPartition>();
                    partition->name = sub_name;
                    partition->device_path = "/dev/" + sub_name;
                    partition->udisks_path = "/org/freedesktop/UDisks2/block_devices/" + sub_name;
                    
                    std::string p_size_str = read_file(sub_entry.path().string() + "/size");
                    if (!p_size_str.empty())
                    {
                        try {
                            partition->capacity = std::stoull(p_size_str) * 512;
                        } catch (...) {
                            partition->capacity = 0;
                        }
                    }

                    std::string p_start_str = read_file(sub_entry.path().string() + "/start");
                    if (!p_start_str.empty())
                    {
                        try {
                            partition->offset = std::stoull(p_start_str) * 512;
                        } catch (...) {
                            partition->offset = 0;
                        }
                    }
                    
                    device->partitions.push_back(std::move(partition));
                }
            }

            m_devices.push_back(std::move(device));
        }
    }

    void DiskManager::scan_mounts()
    {
        struct libmnt_table *tb = mnt_new_table();
        if (!tb) return;

        if (mnt_table_parse_mtab(tb, nullptr) != 0)
        {
            mnt_unref_table(tb);
            return;
        }

        struct libmnt_iter *itr = mnt_new_iter(MNT_ITER_FORWARD);
        struct libmnt_fs *fs;

        while (mnt_table_next_fs(tb, itr, &fs) == 0)
        {
            const char *src = mnt_fs_get_source(fs);
            const char *target = mnt_fs_get_target(fs);
            const char *type = mnt_fs_get_fstype(fs);

            if (!src || !target) continue;

            // Search for this partition in our devices
            for (auto &device : m_devices)
            {
                for (auto &partition : device->partitions)
                {
                    if (partition->device_path == src)
                    {
                        partition->is_mounted = true;
                        partition->mount_point = target;
                        partition->filesystem = type ? type : "unknown";

                        // Get usage info
                        struct statvfs vfs;
                        if (statvfs(target, &vfs) == 0)
                        {
                            partition->capacity = (uint64_t)vfs.f_blocks * vfs.f_frsize;
                            partition->used = partition->capacity - ((uint64_t)vfs.f_bfree * vfs.f_frsize);
                        }
                    }
                }
            }
        }

        mnt_free_iter(itr);
        mnt_unref_table(tb);
    }

    OperationResult DiskManager::mount_partition(const std::string& device_path, const std::string& /*mount_point*/)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // Search for the partition object to get its udisks_path
        std::string udisks_path;
        for (const auto& device : m_devices)
        {
            for (const auto& partition : device->partitions)
            {
                if (partition->device_path == device_path)
                {
                    udisks_path = partition->udisks_path;
                    break;
                }
            }
            if (!udisks_path.empty()) break;
        }

        if (udisks_path.empty() || !m_dbus_helper)
        {
            return {false, "Could not find partition or D-Bus not available"};
        }

        try {
            m_dbus_helper->call_void_method_with_empty_dict(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Filesystem",
                "Mount"
            );
            return {true, "Mount signal sent via UDisks2"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Mount error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

    OperationResult DiskManager::unmount_partition(const std::string& device_path)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::string udisks_path;
        for (const auto& device : m_devices)
        {
            for (const auto& partition : device->partitions)
            {
                if (partition->device_path == device_path)
                {
                    udisks_path = partition->udisks_path;
                    break;
                }
            }
            if (!udisks_path.empty()) break;
        }

        if (udisks_path.empty() || !m_dbus_helper)
        {
            return {false, "Could not find partition or D-Bus not available"};
        }

        try {
            m_dbus_helper->call_void_method_with_empty_dict(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Filesystem",
                "Unmount"
            );
            return {true, "Unmount signal sent via UDisks2"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Unmount error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

    OperationResult DiskManager::format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::string mkfs_tool = "mkfs." + fs_type;
        std::string label_flag = "-L";
        if (fs_type == "vfat" || fs_type == "fat16" || fs_type == "fat32") label_flag = "-n";
        
        std::string final_fs = fs_type;
        if (fs_type == "fat32" || fs_type == "fat16") final_fs = "vfat";

        std::string cmd = "mkfs." + final_fs + " " + label_flag + " \"" + label + "\" " + device_path;
        int res = std::system(cmd.c_str());
        if (res == 0) return {true, "Successfully formatted"};
        return {false, "Failed to format partition"};
    }

    OperationResult DiskManager::eject_device(const std::string& device_path)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::string udisks_path;
        for (const auto& device : m_devices)
        {
            if (device->device_path == device_path)
            {
                udisks_path = device->udisks_path;
                break;
            }
        }

        if (udisks_path.empty() || !m_dbus_helper)
        {
            return {false, "Could not find device or D-Bus not available"};
        }

        try {
            // 1. Get the 'Drive' property from the block device
            auto drive_variant = m_dbus_helper->get_property(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Block",
                "Drive"
            );

            std::string drive_path;
            if (std::holds_alternative<std::string>(drive_variant))
            {
                drive_path = std::get<std::string>(drive_variant);
            }

            if (drive_path.empty() || drive_path == "/")
            {
                return {false, "Device is not an ejectable drive (no Drive property)"};
            }

            // 2. Call Eject on the Drive object
            m_dbus_helper->call_void_method_with_empty_dict(
                "org.freedesktop.UDisks2",
                drive_path,
                "org.freedesktop.UDisks2.Drive",
                "Eject"
            );
            return {true, "Eject signal sent to drive " + drive_path};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Eject error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

    OperationResult DiskManager::erase_disk(const std::string& device_path, const std::string& fs_type, const std::string& label)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        DiskDevice* disk = nullptr;
        for (const auto& d : m_devices)
        {
            if (d->device_path == device_path) {
                disk = d.get();
                break;
            }
        }

        if (!disk || !m_dbus_helper) return {false, "Device not found or D-Bus error"};

        try {
            // 1. Format disk as GPT
            m_dbus_helper->call_method_s_asv(
                "org.freedesktop.UDisks2",
                disk->udisks_path,
                "org.freedesktop.UDisks2.Block",
                "Format",
                "gpt",
                {},
                120000 // 120s timeout
            );

            // 2. Create and format a single partition using the whole capacity
            // ALIGNMENT: Start at 1MiB (2048 sectors) to avoid GPT header conflict.
            uint64_t offset = 1048576; // 1MiB
            uint64_t size = (disk->capacity > offset * 2) ? (disk->capacity - offset * 2) : 0;

            if (size == 0) return {false, "Disk too small for partition"};

            std::map<std::string, dbusutils::DbusVariant> options;
            std::map<std::string, dbusutils::DbusVariant> format_options;
            format_options["label"] = label;

            std::string final_fs = fs_type;
            if (fs_type == "fat32" || fs_type == "fat16") final_fs = "vfat";

            m_dbus_helper->call_method_ttss_asv_s_asv(
                "org.freedesktop.UDisks2",
                disk->udisks_path,
                "org.freedesktop.UDisks2.PartitionTable",
                "CreatePartitionAndFormat",
                offset,
                size,
                "0FC63130-3568-4127-822E-C3DC2671822F", // Linux Filesystem Data GUID for GPT
                "",     // partition name (unused here)
                options,
                final_fs,
                format_options,
                120000 // 120s timeout
            );

            return {true, "Successfully erased disk and created new partition"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Erase disk error: " << e.what();
            return {false, std::string("D-Bus error during erase: ") + e.what()};
        }
    }

    OperationResult DiskManager::recreate_and_format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        DiskPartition* part = nullptr;
        DiskDevice* parent_disk = nullptr;
        
        for (const auto& d : m_devices)
        {
            for (const auto& p : d->partitions)
            {
                if (p->device_path == device_path) {
                    part = p.get();
                    parent_disk = d.get();
                    break;
                }
            }
            if (part) break;
        }

        if (!part || !parent_disk || !m_dbus_helper) return {false, "Partition not found"};

        try {
            uint64_t offset = part->offset;
            uint64_t size = part->capacity;

            // 1. Delete existing partition
            m_dbus_helper->call_void_method_with_empty_dict(
                "org.freedesktop.UDisks2",
                part->udisks_path,
                "org.freedesktop.UDisks2.Partition",
                "Delete"
            );

            // 2. Recreate and format in the same place
            std::map<std::string, dbusutils::DbusVariant> options;
            std::map<std::string, dbusutils::DbusVariant> format_options;
            format_options["label"] = label;

            std::string final_fs = fs_type;
            if (fs_type == "fat32" || fs_type == "fat16") final_fs = "vfat";

            m_dbus_helper->call_method_ttss_asv_s_asv(
                "org.freedesktop.UDisks2",
                parent_disk->udisks_path,
                "org.freedesktop.UDisks2.PartitionTable",
                "CreatePartitionAndFormat",
                offset,
                size,
                "", // Let UDisks2 decide (auto-detects MBR 0x83 or GPT GUID)
                "",
                options,
                final_fs,
                format_options,
                120000 // 120s timeout
            );

            return {true, "Successfully recreated and formatted partition"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Recreate partition error: " << e.what();
            return {false, std::string("D-Bus error during recreation: ") + e.what()};
        }
    }

} // namespace horizon::disks
