#include "horizon/disks/DiskManager.hpp"
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/statvfs.h>
#include <libmount/libmount.h>
#include <iostream>
#include <thread>
#include <chrono>

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
            
            // Second connection for monitoring ONLY to avoid blocking issues
            m_monitor_dbus_helper = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
            m_monitor_dbus_helper->add_match_rule("type='signal',path='/org/freedesktop/UDisks2'");
            m_monitor_dbus_helper->add_match_rule("type='signal',path_namespace='/org/freedesktop/UDisks2'");
            // Listen for job removal to clear state
            m_monitor_dbus_helper->add_match_rule("type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved',path='/org/freedesktop/UDisks2'");
            
            LOG_INFO << "[DiskManager] Monitoring initialized for UDisks2 signals";
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Error: Could not initialize DBusHelper: " << e.what();
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
            std::map<std::string, dbusutils::DbusVariant> options;
            options["force"] = true;

            m_dbus_helper->call_method_s_asv(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Filesystem",
                "Unmount",
                "",
                options
            );
            return {true, "Unmounted (forced) successfully via UDisks2"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Unmount error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

    OperationResult DiskManager::format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        
        // Find the udisks path for the partition
        std::string udisks_path;
        for (const auto& dev : m_devices) {
            for (const auto& part : dev->partitions) {
                if (part->device_path == device_path) {
                    udisks_path = part->udisks_path;
                    break;
                }
            }
            if (!udisks_path.empty()) break;
        }

        if (udisks_path.empty() || !m_dbus_helper) return {false, "Partition not found or D-Bus error"};

        try {
            std::map<std::string, dbusutils::DbusVariant> options;
            options["label"] = label;
            
            std::string final_fs = fs_type;
            if (fs_type == "fat32" || fs_type == "fat16") final_fs = "vfat";

            m_dbus_helper->call_method_s_asv(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Block",
                "Format",
                final_fs,
                options,
                60000 // 60s timeout
            );
            return {true, "Successfully formatted partition via UDisks2"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Format error: " << e.what();
            return {false, std::string("D-Bus format error: ") + e.what()};
        }
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
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_watching_device_path = device_path;
            m_active_job_path = "";
        }

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
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_watching_device_path = device_path;
            m_active_job_path = "";
        }

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

    std::vector<FilesystemInfo> DiskManager::get_supported_filesystems()
    {
        std::vector<FilesystemInfo> supported;
        
        struct KnownFS {
            std::string id;
            std::string name;
            std::string binary;
        };

        std::vector<KnownFS> known = {
            {"ext4", "Linux Extended (EXT4)", "mkfs.ext4"},
            {"ext3", "Linux Extended (EXT3)", "mkfs.ext3"},
            {"ext2", "Linux Extended (EXT2)", "mkfs.ext2"},
            {"ntfs", "Windows NT (NTFS)", "mkfs.ntfs"},
            {"exfat", "Soporte Universal (exFAT)", "mkfs.exfat"},
            {"fat32", "MS-DOS (FAT32)", "mkfs.vfat"},
            {"btrfs", "Btrfs (Copy-on-write)", "mkfs.btrfs"},
            {"xfs", "XFS (High Performance)", "mkfs.xfs"},
            {"f2fs", "Flash-Friendly (F2FS)", "mkfs.f2fs"}
        };

        namespace fs = std::filesystem;
        std::vector<std::string> search_paths = {"/usr/sbin/", "/sbin/", "/usr/bin/", "/bin/"};

        for (const auto& fmt : known) {
            bool found = false;
            for (const auto& path : search_paths) {
                if (fs::exists(path + fmt.binary)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                supported.push_back({fmt.id, fmt.name});
            }
        }

        return supported;
    }

    void DiskManager::process_jobs()
    {
        if (m_watching_device_path.empty() || !m_monitor_dbus_helper) return;

        std::string target_basename = m_watching_device_path;
        if (target_basename.find_last_of('/') != std::string::npos) {
            target_basename = target_basename.substr(target_basename.find_last_of('/') + 1);
        }

        DBusMessage* msg;
        while ((msg = m_monitor_dbus_helper->pop_message(10)))
        {
            const char* path = dbus_message_get_path(msg);
            const char* interface = dbus_message_get_interface(msg);
            const char* member = dbus_message_get_member(msg);

            LOG_INFO << "[DiskMonitor] Signal: " << (interface ? interface : "?") 
                     << "." << (member ? member : "?") << " on " << (path ? path : "?");

            // --- 1. New Jobs (InterfacesAdded) ---
            if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded"))
            {
                DBusMessageIter iter, array_iter;
                dbus_message_iter_init(msg, &iter);
                const char* object_path;
                dbus_message_iter_get_basic(&iter, &object_path);
                dbus_message_iter_next(&iter);
                
                dbus_message_iter_recurse(&iter, &array_iter);
                while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
                {
                    DBusMessageIter entry_iter, props_array_iter;
                    dbus_message_iter_recurse(&array_iter, &entry_iter);
                    const char* iface;
                    dbus_message_iter_get_basic(&entry_iter, &iface);
                    dbus_message_iter_next(&entry_iter);
                    
                    if (std::string(iface).find("Job") != std::string::npos)
                    {
                        bool is_match = false;
                        std::string operation = "";

                        dbus_message_iter_recurse(&entry_iter, &props_array_iter);
                        while (dbus_message_iter_get_arg_type(&props_array_iter) != DBUS_TYPE_INVALID)
                        {
                            DBusMessageIter p_entry_iter, variant_iter;
                            dbus_message_iter_recurse(&props_array_iter, &p_entry_iter);
                            const char* key;
                            dbus_message_iter_get_basic(&p_entry_iter, &key);
                            dbus_message_iter_next(&p_entry_iter);
                            
                            if (std::string(key) == "Objects")
                            {
                                dbus_message_iter_recurse(&p_entry_iter, &variant_iter);
                                if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY)
                                {
                                    DBusMessageIter obj_array_iter;
                                    dbus_message_iter_recurse(&variant_iter, &obj_array_iter);
                                    while (dbus_message_iter_get_arg_type(&obj_array_iter) != DBUS_TYPE_INVALID)
                                    {
                                        const char* obj_path;
                                        dbus_message_iter_get_basic(&obj_array_iter, &obj_path);
                                        if (std::string(obj_path).find(target_basename) != std::string::npos) is_match = true;
                                        dbus_message_iter_next(&obj_array_iter);
                                    }
                                }
                            }
                            else if (std::string(key) == "Operation")
                            {
                                dbus_message_iter_recurse(&p_entry_iter, &variant_iter);
                                if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_STRING) {
                                    const char* op;
                                    dbus_message_iter_get_basic(&variant_iter, &op);
                                    operation = op;
                                }
                            }
                            dbus_message_iter_next(&props_array_iter);
                        }
                        if (is_match)
                        {
                            LOG_INFO << "[DiskMonitor] Linked Job " << object_path << " (" << operation << ") to " << target_basename;
                            m_active_job_path = object_path;
                            m_active_job_operation = operation;
                            // Notify NEW stage started (0%)
                            if (m_progress_cb) m_progress_cb(0.0f, m_active_job_operation);
                        }
                    }
                    dbus_message_iter_next(&array_iter);
                }
            }
            // --- 2. Job Finished (InterfacesRemoved) ---
            else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved"))
            {
                DBusMessageIter iter;
                dbus_message_iter_init(msg, &iter);
                const char* object_path;
                dbus_message_iter_get_basic(&iter, &object_path);

                if (!m_active_job_path.empty() && m_active_job_path == object_path)
                {
                    LOG_INFO << "[DiskMonitor] Job " << object_path << " finished. Forcing 100%.";
                    if (m_progress_cb) m_progress_cb(1.0f, m_active_job_operation);
                    m_active_job_path = "";
                    // Keep operation name until next stage starts
                }
            }
            // --- 3. Progress Updates (PropertiesChanged) ---
            else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
            {
                if (!m_active_job_path.empty() && m_active_job_path == path)
                {
                    DBusMessageIter iter;
                    dbus_message_iter_init(msg, &iter);
                    const char* iface_name;
                    dbus_message_iter_get_basic(&iter, &iface_name);
                    dbus_message_iter_next(&iter);
                    
                    DBusMessageIter dict_iter;
                    dbus_message_iter_recurse(&iter, &dict_iter);
                    
                    while (dbus_message_iter_get_arg_type(&dict_iter) != DBUS_TYPE_INVALID)
                    {
                        DBusMessageIter entry_iter, variant_iter;
                        dbus_message_iter_recurse(&dict_iter, &entry_iter);
                        const char* key;
                        dbus_message_iter_get_basic(&entry_iter, &key);
                        dbus_message_iter_next(&entry_iter);
                        
                        LOG_INFO << "[DiskMonitor] Changed key on " << iface_name << ": " << key;
                        
                        if (std::string(key) == "Progress")
                        {
                            dbus_message_iter_recurse(&entry_iter, &variant_iter);
                            int type = dbus_message_iter_get_arg_type(&variant_iter);
                            double progress = -1.0;

                            if (type == DBUS_TYPE_DOUBLE)
                                dbus_message_iter_get_basic(&variant_iter, &progress);
                            else if (type == DBUS_TYPE_UINT64) {
                                uint64_t val;
                                dbus_message_iter_get_basic(&variant_iter, &val);
                                progress = static_cast<double>(val);
                            }
                            else if (type == DBUS_TYPE_UINT32) {
                                uint32_t val;
                                dbus_message_iter_get_basic(&variant_iter, &val);
                                progress = static_cast<double>(val);
                            }
                            else if (type == DBUS_TYPE_INT32) {
                                int32_t val;
                                dbus_message_iter_get_basic(&variant_iter, &val);
                                progress = static_cast<double>(val);
                            }

                            if (progress >= 0.0 && m_progress_cb)
                            {
                                float p = static_cast<float>(progress);
                                if (p > 1.0f) p /= 100.0f;
                                m_progress_cb(p, m_active_job_operation);
                            }
                        }
                        dbus_message_iter_next(&dict_iter);
                    }
                }
            }
            dbus_message_unref(msg);
        }
    }

    bool DiskManager::check_hardware_changes()
    {
        if (!m_monitor_dbus_helper) return false;

        bool changed = false;
        DBusMessage* msg;
        while ((msg = m_monitor_dbus_helper->pop_message(10)))
        {
            const char* iface = dbus_message_get_interface(msg);
            const char* member = dbus_message_get_member(msg);
            
            LOG_INFO << "[DiskManager] Monitor received signal: " << (iface ? iface : "?") << "." << (member ? member : "?");

            if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved"))
            {
                LOG_INFO << "[DiskManager] Hardware change detected (InterfacesAdded/Removed)";
                changed = true;
            }
            dbus_message_unref(msg);
        }

        if (changed)
        {
            HardwareChangedContext ctx;
            when_hardware_changed.run(ctx);
        }
        return changed;
    }

    OperationResult DiskManager::unmount_all_partitions(const std::string& device_path)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        LOG_INFO << "[DiskManager] Ensuring all partitions on " << device_path << " are unmounted";
        
        // Find the device and its partitions
        DiskDevice* target_dev = nullptr;
        for (const auto& dev : m_devices) {
            if (dev->device_path == device_path) {
                target_dev = dev.get();
                break;
            }
        }

        if (!target_dev) return {false, "Device not found"};

        for (const auto& part : target_dev->partitions) {
            if (part->is_mounted) {
                LOG_INFO << "[DiskManager] Unmounting " << part->device_path << " (Forced)";
                unmount_partition(part->device_path);
            }
        }

        // Give the kernel and udev some time to settle
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        return {true, "Partitions unmounted (forced)"};
    }

    OperationResult DiskManager::create_partition_table(const std::string& device_path, const std::string& type)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        LOG_INFO << "[DiskManager] Creating partition table " << type << " on " << device_path;
        
        std::string udisks_path;
        for (const auto& dev : m_devices) {
            if (dev->device_path == device_path) {
                udisks_path = dev->udisks_path;
                break;
            }
        }

        if (udisks_path.empty() || !m_dbus_helper) return {false, "Device not found"};

        try {
            std::map<std::string, dbusutils::DbusVariant> options;
            m_dbus_helper->call_method_s_asv(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.Block", // Correct interface for Format
                "Format",
                type,
                options,
                30000 // 30s timeout
            );
            LOG_INFO << "[DiskManager] Partition table created successfully";
            return {true, "Partition table created"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Create partition table error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

    OperationResult DiskManager::create_partition(const std::string& device_path, uint64_t offset, uint64_t size, const std::string& type, const std::string& name, uint64_t flags)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        LOG_INFO << "[DiskManager] Creating partition at " << offset << " (" << size << " bytes) on " << device_path;
        
        std::string udisks_path;
        for (const auto& dev : m_devices) {
            if (dev->device_path == device_path) {
                udisks_path = dev->udisks_path;
                break;
            }
        }

        if (udisks_path.empty() || !m_dbus_helper) return {false, "Device not found"};

        try {
            std::map<std::string, dbusutils::DbusVariant> options;
            if (flags != 0) {
                options["flags"] = flags;
            }

            m_dbus_helper->call_method_ttss_asv(
                "org.freedesktop.UDisks2",
                udisks_path,
                "org.freedesktop.UDisks2.PartitionTable",
                "CreatePartition",
                offset,
                size,
                type,
                name,
                options,
                30000 // 30s timeout
            );
            LOG_INFO << "[DiskManager] Partition created successfully";
            return {true, "Partition created"};
        } catch (const std::exception& e) {
            LOG_ERROR << "[DiskManager] Create partition error: " << e.what();
            return {false, std::string("D-Bus error: ") + e.what()};
        }
    }

} // namespace horizon::disks
