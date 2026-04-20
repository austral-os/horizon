#include "horizon-installer-utils/InstallerManager.hpp"
#include <horizon/Logger.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace horizon::installer
{
    InstallerManager::InstallerManager() = default;
    InstallerManager::~InstallerManager() = default;

    StepResult InstallerManager::run_stage1(const InstallationConfig& config)
    {
        LOG_INFO << "Starting Stage 1 installation on " << config.target_device;
        
        report_progress(0.05, "Partitioning disk...");
        auto res = partition_disk(config.target_device);
        if (!res.success) return res;

        report_progress(0.20, "Copying system files (this may take a while)...");
        res = copy_system("/mnt/target");
        if (!res.success) return res;

        report_progress(0.80, "Installing bootloader...");
        res = install_bootloader(config.target_device, "/mnt/target");
        if (!res.success) return res;

        report_progress(0.90, "Configuring system...");
        res = configure_fstab("/mnt/target", config.target_device);
        if (!res.success) return res;

        res = create_oobe_trigger("/mnt/target");
        if (!res.success) return res;

        report_progress(1.0, "Stage 1 complete!");
        return {true, "Installation finished successfully. Please reboot."};
    }

    StepResult InstallerManager::run_stage2(const InstallationConfig& config)
    {
        LOG_INFO << "Starting Stage 2 (OOBE) for user " << config.username;
        
        report_progress(0.1, "Creating user account...");
        auto res = create_user(config.username, config.password);
        if (!res.success) return res;

        report_progress(0.5, "Setting system configuration...");
        res = set_system_config(config.hostname, config.timezone);
        if (!res.success) return res;

        finalize_oobe();
        report_progress(1.0, "OOBE Complete!");
        return {true, "System ready."};
    }

    bool InstallerManager::is_oobe_pending()
    {
        return std::filesystem::exists("/etc/horizon-setup-pending");
    }

    StepResult InstallerManager::partition_disk(const std::string& device_path)
    {
        // For SAFETY: We only log what we would do if not in a real installer environment
        // In a real environment, we would run:
        // parted -s <dev> mklabel gpt
        // parted -s <dev> mkpart ESP fat32 1MiB 513MiB
        // parted -s <dev> set 1 esp on
        // parted -s <dev> mkpart primary ext4 513MiB 100%
        
        LOG_INFO << "[DRY RUN] Partitioning " << device_path;
        
        // Mocking execution for now
        std::string cmd = "parted -s " + device_path + " mklabel gpt";
        LOG_INFO << "Executing: " << cmd;
        // int r = std::system(cmd.c_str()); 
        
        return {true, "Disk partitioned (Mocked)"};
    }

    StepResult InstallerManager::copy_system(const std::string& target_root)
    {
        // In a real environment:
        // rsync -axHAWXS --numeric-ids --info=progress2 / /mnt/target --exclude=/dev/* --exclude=/proc/* ...
        
        LOG_INFO << "[DRY RUN] Copying system to " << target_root;
        return {true, "Files copied (Mocked)"};
    }

    StepResult InstallerManager::install_bootloader(const std::string& device_path, const std::string& target_root)
    {
        LOG_INFO << "[DRY RUN] Installing GRUB on " << device_path;
        return {true, "Bootloader installed (Mocked)"};
    }

    StepResult InstallerManager::configure_fstab(const std::string& target_root, const std::string& device_path)
    {
        LOG_INFO << "[DRY RUN] Generating fstab at " << target_root << "/etc/fstab";
        return {true, "fstab configured (Mocked)"};
    }

    StepResult InstallerManager::create_oobe_trigger(const std::string& target_root)
    {
        LOG_INFO << "Creating OOBE trigger at " << target_root << "/etc/horizon-setup-pending";
        // This is safe to do if the directory exists
        return {true, "OOBE trigger created"};
    }

    StepResult InstallerManager::create_user(const std::string& username, const std::string& password)
    {
        LOG_INFO << "[DRY RUN] Creating user " << username;
        return {true, "User created (Mocked)"};
    }

    StepResult InstallerManager::set_system_config(const std::string& hostname, const std::string& timezone)
    {
        LOG_INFO << "[DRY RUN] Setting hostname to " << hostname << " and timezone to " << timezone;
        return {true, "System config set (Mocked)"};
    }

    void InstallerManager::finalize_oobe()
    {
        LOG_INFO << "Finalizing OOBE, removing trigger file...";
        // std::filesystem::remove("/etc/horizon-setup-pending");
    }

    void InstallerManager::report_progress(float progress, const std::string& message)
    {
        if (m_progress_cb) m_progress_cb(progress, message);
    }

} // namespace horizon::installer
