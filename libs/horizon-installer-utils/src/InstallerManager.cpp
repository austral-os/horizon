#include "horizon-installer-utils/InstallerManager.hpp"
#include <horizon/Logger.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace horizon::installer
{
    InstallerManager::InstallerManager()
    {
        // Bridge m_disk_manager progress to our own
        m_disk_manager.set_progress_callback(
            [this](float progress, const std::string& message)
            {
                report_progress(progress, message);
            });
    }
    
    InstallerManager::~InstallerManager() = default;

    StepResult InstallerManager::run_stage1(const InstallationConfig& config)
    {
        std::string clean_device = sanitize_device_path(config.target_device);
        LOG_INFO << "Starting Stage 1 installation on " << clean_device << " (Original: " << config.target_device << ")";
        
        report_progress(0.05, "Partitioning disk...");
        auto res = partition_disk(clean_device);
        if (!res.success) return res;

        report_progress(0.20, "Copying system files (this may take a while)...");
        res = copy_system(m_target_mount_point);
        if (!res.success) return res;

        // Ensure data is written before installing bootloader
        execute_privileged_command("/usr/bin/sync");

        report_progress(0.80, "Installing bootloader...");
        res = install_bootloader(clean_device, m_target_mount_point);
        if (!res.success) return res;

        report_progress(0.90, "Configuring system...");
        res = configure_fstab(m_target_mount_point, clean_device);
        if (!res.success) return res;

        res = create_oobe_trigger(m_target_mount_point);
        if (!res.success) return res;

        // 10. Final Validation
        report_progress(0.95, "Verifying installation...");
        if (!std::filesystem::exists(m_target_mount_point + "/boot/vmlinuz") && 
            !std::filesystem::exists(m_target_mount_point + "/vmlinuz") &&
            std::filesystem::is_empty(m_target_mount_point + "/boot")) {
            LOG_WARNING << "Kernel not found in target! System may not boot.";
        }

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
        LOG_INFO << "Partitioning " << device_path << " natively via UDisks2...";
        
        // 1. Ensure disk is not busy by unmounting partitions
        report_progress(0.05, "Preparing disk (unmounting)...");
        m_disk_manager.unmount_all_partitions(device_path);
        
        // 2. Get disk capacity
        m_disk_manager.scan();
        uint64_t total_capacity = 0;
        for (const auto& dev : m_disk_manager.devices()) {
            if (dev->device_path == device_path) {
                total_capacity = dev->capacity;
                break;
            }
        }

        if (total_capacity == 0) return {false, "Could not determine disk capacity"};

        // 2. Create Partition Table (GPT)
        auto res_op = m_disk_manager.create_partition_table(device_path, "gpt");
        if (!res_op.success) return {false, "Failed to create partition table: " + res_op.message};

        // 3. Create EFI Partition (512MB)
        // Offset: 1MiB (standard alignment)
        // 3. Create EFI Partition (512MB)
        // Offset: 1MiB (standard alignment)
        uint64_t efi_offset = 1048576; 
        uint64_t efi_size = 512 * 1024 * 1024; // 512MiB
        
        // Lowercase GUID is safer for some D-Bus/UDisks2 version matching
        // Flag 0x1: System Partition bit (GPT)
        res_op = m_disk_manager.create_partition(device_path, efi_offset, efi_size, "c12a7328-f81f-11d2-ba4b-00a0c93ec93b", "EFI", 0x1);
        if (!res_op.success) return {false, "Failed to create EFI partition: " + res_op.message};

        // 4. Create Root Partition (Rest of space)
        uint64_t root_offset = efi_offset + efi_size;
        uint64_t root_size = (total_capacity > root_offset + 1048576) ? (total_capacity - root_offset - 1048576) : 0;
        
        if (root_size == 0) return {false, "Disk too small for root partition"};

        // 0FC63130-3568-4127-822E-C3DC2671822F is the Linux filesystem data GUID for GPT
        res_op = m_disk_manager.create_partition(device_path, root_offset, root_size, "0FC63130-3568-4127-822E-C3DC2671822F", "ROOT");
        if (!res_op.success) return {false, "Failed to create root partition: " + res_op.message};

        // 5. Scan again to find the new partitions
        m_disk_manager.scan();

        // 6. Determine partition naming (SATA vs NVMe)
        // We need the ACTUAL device paths for mounting. 
        // Note: create_partition doesn't return the path immediately, so we rely on scan()
        std::string p1, p2;
        if (device_path.find("nvme") != std::string::npos || (device_path.back() >= '0' && device_path.back() <= '9')) {
            p1 = device_path + "p1"; p2 = device_path + "p2";
        } else {
            p1 = device_path + "1"; p2 = device_path + "2";
        }

        // 5. Format Partitions
        report_progress(0.15, "Formatting target volumes...");
        
        // Force FAT32 for EFI. We use mkfs.vfat directly to guarantee -F 32
        auto res_format = execute_privileged_command("/usr/sbin/mkfs.vfat -F 32 -n BOOT " + p1);
        if (!res_format.success) return res_format;

        auto fs_res = m_disk_manager.format_partition(p2, "ext4", "ROOT");
        if (!fs_res.success) return {false, "Failed to format Root: " + fs_res.message};

        // 8. Mount Partitions natively via UDisks2 (Dynamic paths)
        report_progress(0.2, "Mounting target volumes...");
        
        // Root Partition (p2)
        auto mount_res = m_disk_manager.mount_partition(p2, "");
        if (!mount_res.success) return {false, "Failed to mount Root: " + mount_res.message};

        // Refresh to find the mount point
        m_disk_manager.scan();
        
        std::string target_root;
        for (const auto& dev : m_disk_manager.devices()) {
            for (const auto& part : dev->partitions) {
                if (part->device_path == p2) {
                    target_root = part->mount_point;
                    break;
                }
            }
            if (!target_root.empty()) break;
        }

        if (target_root.empty()) return {false, "Failed to resolve root mount point after mounting"};
        LOG_INFO << "Target root dynamically mounted at: " << target_root;

        // EFI Partition (p1)
        mount_res = m_disk_manager.mount_partition(p1, "");
        if (!mount_res.success) return {false, "Failed to mount EFI: " + mount_res.message};

        m_disk_manager.scan();
        std::string target_efi;
        for (const auto& dev : m_disk_manager.devices()) {
            for (const auto& part : dev->partitions) {
                if (part->device_path == p1) {
                    target_efi = part->mount_point;
                    break;
                }
            }
            if (!target_efi.empty()) break;
        }

        if (target_efi.empty()) return {false, "Failed to resolve EFI mount point after mounting"};

        m_target_mount_point = target_root;
        m_efi_mount_point = target_efi;

        // 9. Prepare Mount Structure (Assemble EFI into Root tree for configuration steps)
        // We use pkexec for mkdir because /media is likely root-owned for ext4
        auto res_mkdir = execute_privileged_command("mkdir -p " + m_target_mount_point + "/boot/efi");
        if (!res_mkdir.success) return res_mkdir;
        
        auto res_mount = execute_privileged_command("mount --bind " + m_efi_mount_point + " " + m_target_mount_point + "/boot/efi");
        if (!res_mount.success) LOG_WARNING << "Failed to bind mount EFI: " << res_mount.message;
        
        return {true, "Disk prepared and mounted at " + m_target_mount_point};
    }

    StepResult InstallerManager::copy_system(const std::string& target_root)
    {
        LOG_INFO << "Cloning system to " << target_root << "...";
        
        // We REMOVE -x (one-file-system) to ensure /boot is copied even if it's a separate mount
        // We explicitly exclude pseudo-filesystems and target mount points
        // -aAXHS: preserve all attributes, ACLs, Xattrs, sparse files
        std::string cmd = "/usr/bin/rsync -aAXHS --numeric-ids --info=progress2 / " + target_root + 
                          " --exclude=/dev/* --exclude=/proc/* --exclude=/sys/* " +
                          " --exclude=/tmp/* --exclude=/run/* --exclude=/mnt/* " +
                          " --exclude=/media/* --exclude=/lost+found";
        
        return execute_privileged_command(cmd);
    }

    StepResult InstallerManager::install_bootloader(const std::string& device_path, const std::string& target_root)
    {
        LOG_INFO << "Installing GRUB on " << device_path << " (Root: " << target_root << ")";
        
        // Prepare Virtual Filesystems (Required for GRUB to detect devices and NVRAM inside chroot)
        std::vector<std::string> vfs_folders = {"/dev", "/proc", "/sys", "/run"};
        for (const auto& folder : vfs_folders) {
            execute_privileged_command("mount --bind " + folder + " " + target_root + folder);
        }

        // Special handling for EFI variables (Required for NVRAM registration)
        if (std::filesystem::exists("/sys/firmware/efi/efivars")) {
            execute_privileged_command("mkdir -p " + target_root + "/sys/firmware/efi/efivars");
            execute_privileged_command("mount --bind /sys/firmware/efi/efivars " + target_root + "/sys/firmware/efi/efivars");
        }

        // 1. Install GRUB for EFI (Elevated inside CHROOT)
        // By running inside chroot, we use the target's grub-install binary and environment
        std::string cmd = "/usr/sbin/chroot " + target_root + " /usr/sbin/grub-install --target=x86_64-efi --efi-directory=/boot/efi --boot-directory=/boot --recheck --removable";
        auto res = execute_privileged_command(cmd);
        
        if (res.success) {
            // 2. Refresh System Branding (Initramfs, Plymouth and Alternatives)
            report_progress(0.85, "Enforcing Austral OS visual identity...");

            // 2.a Migrate themes and splash (Final Surgical Migration)
            LOG_INFO << "Surgically migrating branding assets from Live medium...";
            execute_privileged_command("/usr/bin/mkdir -p " + target_root + "/usr/share/grub/themes/austral");
            
            // Mirror the theme folder
            execute_privileged_command("/usr/bin/rsync -va /run/live/medium/boot/grub/live-theme/ " + target_root + "/usr/share/grub/themes/austral/");
            
            // Mirror the splash image (Explicitly needed as it sits outside the theme folder on the ISO)
            execute_privileged_command("/usr/bin/rsync -va /run/live/medium/boot/grub/splash.png " + target_root + "/usr/share/grub/themes/splash.png");
            
            // Sync to ensure files are physically on disk
            execute_privileged_command("/usr/bin/sync");
            
            // Branding Deployment Script:
            // 1. Detect themes in filesystem
            // 2. Set plymouth theme
            // 3. Set update-alternatives for desktop-base
            // 4. Inject GRUB branding into /etc/default/grub
            std::string branding_script = "/usr/sbin/chroot " + target_root + " bash -c '"
                "echo \"[Diagnostics] Verifying branding assets on target disk...\"; "
                "ls -l /usr/share/grub/themes/splash.png 2>/dev/null; "
                "ls -R /usr/share/grub/themes/austral/ 2>/dev/null; "
                
                "echo \"[Identity] Activating Austral OS visual themes...\"; "
                "P_THEME=$(ls /usr/share/plymouth/themes | grep -E \"austral|horizon|os\" | head -n 1); "
                "D_THEME=$(ls /usr/share/desktop-base | grep -E \"austral|horizon|os\" | head -n 1); "
                "if [ ! -z \"$P_THEME\" ]; then echo \"[Plymouth] Setting theme: $P_THEME\"; plymouth-set-default-theme $P_THEME; fi; "
                "if [ ! -z \"$D_THEME\" ]; then "
                    "echo \"[Desktop] Setting theme alternatives: $D_THEME\"; "
                    "update-alternatives --set desktop-theme /usr/share/desktop-base/$D_THEME 2>/dev/null; "
                "fi; "
                
                "echo \"[GRUB] Injecting hardened configuration parameters...\"; "
                "if ! grep -q \"GRUB_CMDLINE_LINUX_DEFAULT.*splash\" /etc/default/grub; then "
                    "sed -i \"s/GRUB_CMDLINE_LINUX_DEFAULT=\\\"/GRUB_CMDLINE_LINUX_DEFAULT=\\\"quiet splash /\" /etc/default/grub; "
                "fi; "
                "sed -i \"s/quiet quiet/quiet/g; s/splash splash/splash/g\" /etc/default/grub; "
                "sed -i \"s/^#GRUB_GFXMODE=.*/GRUB_GFXMODE=auto/\" /etc/default/grub; "
                "sed -i \"s/^GRUB_TERMINAL=console/#GRUB_TERMINAL=console/\" /etc/default/grub; "

                "G_THEME=\\\"/usr/share/grub/themes/austral/theme.txt\\\"; "
                "G_BACK=\\\"/usr/share/grub/themes/splash.png\\\"; "
                
                "echo \"[GRUB] Forcing GRUB_THEME and GRUB_BACKGROUND...\"; "
                "grep -q \"^GRUB_THEME=\" /etc/default/grub && sed -i \"s|^GRUB_THEME=.*|GRUB_THEME=$G_THEME|\" /etc/default/grub || "
                "(grep -q \"^#GRUB_THEME=\" /etc/default/grub && sed -i \"s|^#GRUB_THEME=.*|GRUB_THEME=$G_THEME|\" /etc/default/grub || echo \"GRUB_THEME=$G_THEME\" >> /etc/default/grub); "
                
                "grep -q \"^GRUB_BACKGROUND=\" /etc/default/grub && sed -i \"s|^GRUB_BACKGROUND=.*|GRUB_BACKGROUND=$G_BACK|\" /etc/default/grub || "
                "(grep -q \"^#GRUB_BACKGROUND=\" /etc/default/grub && sed -i \"s|^#GRUB_BACKGROUND=.*|GRUB_BACKGROUND=$G_BACK|\" /etc/default/grub || echo \"GRUB_BACKGROUND=$G_BACK\" >> /etc/default/grub); "
                
                "echo \"[Diagnostics] Final /etc/default/grub branding state:\"; "
                "cat /etc/default/grub | grep -E \"GRUB_THEME|GRUB_BACKGROUND|GRUB_CMDLINE|GRUB_GFXMODE\"'";
            
            execute_privileged_command(branding_script);

            // Update initramfs to bake the detected theme into boot
            execute_privileged_command("/usr/sbin/chroot " + target_root + " /usr/sbin/update-initramfs -u");

            // 3. Generate GRUB config using chroot (Elevated)
            // We run this AFTER initramfs and branding injection to ensure it detects the new visual environment
            res = execute_privileged_command("/usr/sbin/chroot " + target_root + " /usr/sbin/update-grub");
        }

        // Cleanup Virtual Filesystems (LIFO order for unmounting)
        if (std::filesystem::exists("/sys/firmware/efi/efivars")) {
             execute_privileged_command("umount -l " + target_root + "/sys/firmware/efi/efivars");
        }
        for (auto it = vfs_folders.rbegin(); it != vfs_folders.rend(); ++it) {
            execute_privileged_command("umount -l " + target_root + *it);
        }

        if (!res.success) return res;
        
        // Final sync for bootloader files
        execute_privileged_command("/usr/bin/sync");
        
        return {true, "Bootloader installed and configured successfully"};
    }

    StepResult InstallerManager::configure_fstab(const std::string& target_root, const std::string& device_path)
    {
        LOG_INFO << "Generating fstab at " << target_root << "/etc/fstab";
        
        // Determine partition naming (same logic as partition_disk)
        std::string p1 = device_path;
        std::string p2 = device_path;
        if (device_path.find("nvme") != std::string::npos || (device_path.back() >= '0' && device_path.back() <= '9')) {
            p1 += "p1"; p2 += "p2";
        } else {
            p1 += "1"; p2 += "2";
        }

        // Helper to get UUID (Privileged)
        auto get_uuid = [this](const std::string& part) -> std::string {
            std::string temp_file = "/tmp/uuid_" + std::filesystem::path(part).filename().string() + ".tmp";
            std::string cmd = "/usr/sbin/blkid -s UUID -o value '" + part + "' > " + temp_file;
            
            // blkid usually needs root to read filesystem superblocks
            auto res = execute_privileged_command(cmd);
            if (!res.success) return "";

            std::ifstream file(temp_file);
            std::string uuid;
            if (file >> uuid) {
                std::filesystem::remove(temp_file);
                return uuid;
            }
            return "";
        };

        std::string uuid_root = get_uuid(p2);
        std::string uuid_efi = get_uuid(p1);

        if (uuid_root.empty()) return {false, "Failed to get UUID for root partition"};

        // Generate fstab content in a string and write to a temporary file
        std::string fstab_content = "# <file system> <mount point>   <type>  <options>       <dump>  <pass>\n"
                                   "UUID=" + uuid_root + " /               ext4    errors=remount-ro 0       1\n";
        if (!uuid_efi.empty()) {
            fstab_content += "UUID=" + uuid_efi + " /boot/efi       vfat    umask=0077      0       2\n";
        }

        std::string temp_fstab = "/tmp/fstab.installer";
        std::ofstream file(temp_fstab);
        if (file.is_open()) {
            file << fstab_content;
            file.close();
        }

        // Copy fstab to the target location with elevated privileges
        return execute_privileged_command("/usr/bin/cp " + temp_fstab + " " + target_root + "/etc/fstab");
    }

    StepResult InstallerManager::create_oobe_trigger(const std::string& target_root)
    {
        LOG_INFO << "Creating OOBE trigger at " << target_root << "/etc/horizon-setup-pending";
        
        // Ensure directory exists (elevated)
        execute_privileged_command("/usr/bin/mkdir -p " + target_root + "/etc");
        
        // Create the file (elevated)
        return execute_privileged_command("/usr/bin/touch " + target_root + "/etc/horizon-setup-pending");
    }

    StepResult InstallerManager::create_user(const std::string& username, const std::string& password)
    {
        LOG_INFO << "Creating user " << username << "...";
        
        // Use execute_privileged_command for user management
        auto res = execute_privileged_command("/usr/sbin/useradd -m -G sudo,video,audio,input -s /bin/bash " + username);
        if (!res.success) return res;

        std::string cmd = "echo \"" + username + ":" + password + "\" | /usr/sbin/chpasswd";
        return execute_privileged_command(cmd);
    }

    StepResult InstallerManager::set_system_config(const std::string& hostname, const std::string& timezone)
    {
        LOG_INFO << "Setting hostname to " << hostname << " and timezone to " << timezone;
        
        auto res = execute_privileged_command("/usr/bin/hostnamectl set-hostname " + hostname);
        if (!res.success) return res;

        return execute_privileged_command("/usr/bin/timedatectl set-timezone " + timezone);
    }

    void InstallerManager::finalize_oobe()
    {
        LOG_INFO << "Finalizing OOBE, removing trigger file...";
        std::filesystem::remove("/etc/horizon-setup-pending");
    }

    void InstallerManager::report_progress(float progress, const std::string& message)
    {
        if (m_progress_cb) m_progress_cb(progress, message);
    }

    StepResult InstallerManager::execute_command(const std::string& command)
    {
        LOG_INFO << "Executing: " << command;
        
        // Append 2>&1 to capture error output in the pipe
        std::string full_cmd = command + " 2>&1";
        FILE* pipe = popen(full_cmd.c_str(), "r");
        if (!pipe) {
            return {false, "Failed to start command: " + command};
        }

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string line(buffer);
            
            // Simple progress parsing for rsync --info=progress2
            // Format: "          0   0%    0.00kB/s    0:00:00"
            size_t pos = line.find("%");
            if (pos != std::string::npos && pos > 2) {
                try {
                    size_t start = pos - 1;
                    while (start > 0 && std::isdigit(line[start])) start--;
                    
                    if (start < pos - 1) {
                        int percent = std::stoi(line.substr(start + 1, pos - start - 1));
                        
                        // Map rsync 0-100% to global 20%-80%
                        if (command.find("rsync") != std::string::npos) {
                            float global_prog = 0.20f + (percent / 100.0f) * 0.60f;
                            report_progress(global_prog, "Copying system files (" + std::to_string(percent) + "%)...");
                        }
                    }
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        }

        int status = pclose(pipe);
        if (status == 0) {
            return {true, "Command executed successfully"};
        } else {
            LOG_ERROR << "Command failed with status " << status << ": " << command;
            return {false, "Command failed with status " + std::to_string(status)};
        }
    }

    std::string InstallerManager::sanitize_device_path(const std::string& path)
    {
        if (path.empty()) return "";

        // Use DiskManager to find a matching device
        m_disk_manager.scan();
        
        // Try exact match first
        for (const auto& dev : m_disk_manager.devices()) {
            if (dev->device_path == path || dev->name == path) {
                return dev->device_path;
            }
        }

        // Try fuzzy match (e.g., if input is "sda (30 GB)")
        for (const auto& dev : m_disk_manager.devices()) {
            if (path.find(dev->name) != std::string::npos) {
                LOG_INFO << "Found matching device: " << dev->device_path << " for input: " << path;
                return dev->device_path;
            }
        }

        LOG_WARNING << "Could not resolve device path for: " << path << ". Returning as is.";
        return path;
    }

    StepResult InstallerManager::execute_privileged_command(const std::string& command)
    {
        if (getuid() == 0) {
            return execute_command(command);
        } else {
            LOG_INFO << "[Privileged] Requesting elevation for: " << command;
            return execute_command("pkexec " + command);
        }
    }

} // namespace horizon::installer
