#include "horizon-disk-utilities/DiskManager.hpp"
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

    DiskManager::DiskManager() {}
    DiskManager::~DiskManager() {}

    void DiskManager::scan()
    {
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
                    
                    std::string p_size_str = read_file(sub_entry.path().string() + "/size");
                    if (!p_size_str.empty())
                    {
                        try {
                            partition->capacity = std::stoull(p_size_str) * 512;
                        } catch (...) {
                            partition->capacity = 0;
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

    OperationResult DiskManager::mount_partition(const std::string& device_path, const std::string& mount_point)
    {
        // For now, wrapping shell commands as requested "assume root privileges"
        std::string cmd = "mount " + device_path + " " + mount_point;
        int res = std::system(cmd.c_str());
        if (res == 0) return {true, "Successfully mounted"};
        return {false, "Failed to mount partition"};
    }

    OperationResult DiskManager::unmount_partition(const std::string& device_path)
    {
        std::string cmd = "umount " + device_path;
        int res = std::system(cmd.c_str());
        if (res == 0) return {true, "Successfully unmounted"};
        return {false, "Failed to unmount partition"};
    }

    OperationResult DiskManager::format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label)
    {
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
        std::string cmd = "eject " + device_path;
        int res = std::system(cmd.c_str());
        if (res == 0) return {true, "Successfully ejected"};
        return {false, "Failed to eject device"};
    }

} // namespace horizon::disks
