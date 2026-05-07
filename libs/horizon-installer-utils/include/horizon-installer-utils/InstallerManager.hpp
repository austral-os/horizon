#pragma once

#include <horizon-disk-utilities/DiskManager.hpp>
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace horizon::installer
{
    /**
     * @brief Result of an installation step.
     */
    struct StepResult
    {
        bool success;
        std::string message;
    };

    /**
     * @brief Information about the installation target.
     */
    struct InstallationConfig
    {
        std::string target_device; // e.g., "/dev/sda"
        std::string fullname;      // Stage 2 (OOBE)
        std::string username;      // Stage 2 (OOBE)
        std::string password;      // Stage 2 (OOBE)
        std::string avatar;        // Stage 2 (OOBE)
        std::string hostname;      // Stage 2 (OOBE)
        std::string country;       // Stage 2 (OOBE)
        std::string timezone;      // Stage 2 (OOBE)
        std::string locale;        // Stage 1 & 2
        bool is_oobe = false;      // True if running in Stage 2
    };

    /**
     * @brief Orchestrates the installation and OOBE processes.
     */
    class InstallerManager
    {
    public:
        using ProgressCallback = std::function<void(float, const std::string&)>;

        InstallerManager();
        ~InstallerManager();

        /**
         * @brief Set the callback for progress updates.
         */
        void set_progress_callback(ProgressCallback cb) { m_progress_cb = cb; }

        /**
         * @brief Stage 1: Prepares the disk and installs the system.
         * @param config Configuration for Stage 1 (target_device and locale are essential).
         */
        StepResult run_stage1(const InstallationConfig& config);

        /**
         * @brief Stage 2: Configures the installed system (OOBE).
         * @param config Configuration for Stage 2 (user data, hostname, etc.).
         */
        StepResult run_stage2(const InstallationConfig& config);

        /**
         * @brief Checks if OOBE is required on the current system.
         */
        static bool is_oobe_pending();

        /**
         * @brief Checks if the system setup is already complete.
         */
        static bool is_setup_done();

        /**
         * @brief Marks the system setup as complete.
         * @param root Optional root path (e.g., a mount point).
         */
        void mark_setup_done(const std::string& root = "");

    private:
        ProgressCallback m_progress_cb;
        horizon::disks::DiskManager m_disk_manager;
        std::string m_target_mount_point;
        std::string m_efi_mount_point;

        // Internal steps
        StepResult partition_disk(const std::string& device_path);
        StepResult copy_system(const std::string& target_root);
        StepResult install_bootloader(const std::string& device_path, const std::string& target_root);
        StepResult configure_fstab(const std::string& target_root, const std::string& device_path);
        StepResult create_oobe_trigger(const std::string& target_root);
        
        StepResult create_user(const std::string& username, const std::string& password, const std::string& fullname = "");
        StepResult set_system_config(const std::string& hostname, const std::string& timezone);
        StepResult set_user_avatar(const std::string& username, const std::string& avatar_path);
        void finalize_oobe();

        void report_progress(float progress, const std::string& message);
        
        // System execution helper
        StepResult execute_command(const std::string& command);
        StepResult execute_privileged_command(const std::string& command);
        std::string sanitize_device_path(const std::string& path);
    };
} // namespace horizon::installer
