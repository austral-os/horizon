#pragma once
#include "DiskDevice.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace horizon::dbusutils { class DbusHelper; }

namespace horizon::disks
{
    /**
     * @brief Information about a supported filesystem format.
     */
    struct FilesystemInfo
    {
        std::string id;   // e.g., "ext4"
        std::string name; // e.g., "Linux Extended (EXT4)"
    };

    /**
     * @brief Result of a disk operation.
     */
    struct OperationResult
    {
        bool success;
        std::string message;
    };

    /**
     * @brief Main class for disk management operations.
     */
    class DiskManager
    {
    public:
        DiskManager();
        ~DiskManager();

        /**
         * @brief Scans the system for disks and partitions.
         */
        void scan();

        /**
         * @brief Returns the list of detected disk devices.
         */
        const std::vector<std::unique_ptr<DiskDevice>>& devices() const { return m_devices; }

        /**
         * @brief Returns the list of filesystems supported by the system for formatting.
         */
        static std::vector<FilesystemInfo> get_supported_filesystems();

        /**
         * @brief Mounts a partition.
         */
        OperationResult mount_partition(const std::string& device_path, const std::string& mount_point);

        /**
         * @brief Unmounts a partition.
         */
        OperationResult unmount_partition(const std::string& device_path);

        /**
         * @brief Formats a partition with the specified filesystem.
         */
        OperationResult format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label);

        /**
         * @brief Ejects a removable device.
         */
        OperationResult eject_device(const std::string& device_path);

        /**
         * @brief Erases a whole disk, creating a new partition table and a single partition.
         */
        OperationResult erase_disk(const std::string& device_path, const std::string& fs_type, const std::string& label);

        /**
         * @brief Deletes and recreates a partition with a new format.
         */
        OperationResult recreate_and_format_partition(const std::string& device_path, const std::string& fs_type, const std::string& label);

    private:
        std::vector<std::unique_ptr<DiskDevice>> m_devices;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus_helper;
        mutable std::recursive_mutex m_mutex;
        
        // Internal helpers
        void scan_sys_block();
        void scan_mounts();
    };
} // namespace horizon::disks
