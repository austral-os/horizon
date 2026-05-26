#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/storage/RemoteManager.hpp>
#include <horizon/compression/CompressionTask.hpp>
#include <vector>

namespace horizon
{
    class Label;
    class ProgressBar;
}

namespace horizon::files
    {
        class FileView;
        class FileSidebar;
    }

namespace horizon::storage
    {
        class MountPasswordDialog;
    }

namespace horizon::arkfm
{

    class ArkfmWindow : public ApplicationWindow
    {
    public:
        ArkfmWindow(int w = 1200, int h = 720);
        ~ArkfmWindow() override;

        void handle_rename(const std::string &path);
        void handle_delete(const std::vector<std::string> &paths);
        void handle_open();
        void handle_properties();
        void handle_new_folder();
        void handle_mount_remote(const std::string &uri, storage::RemoteCredentials creds = {}, storage::MountPasswordDialog* dlg = nullptr);


        void handle_toggle_hidden();
        void show_status_message(const std::string &msg, int timeout_ms = 3000);

        void handle_extract(const std::string &path);
        void handle_compress(const std::vector<std::string> &paths, const std::string &format_ext);

    private:
        files::FileView *m_view_ptr{nullptr};
        files::FileSidebar *m_sidebar_ptr{nullptr};
        horizon::Label *m_status_label{nullptr};
        horizon::ProgressBar *m_progress_bar{nullptr};
        std::unique_ptr<horizon::Menu> m_active_context_menu;
        std::unique_ptr<storage::RemoteManager> m_remote_manager;
        std::shared_ptr<storage::MountPasswordDialog> m_mount_dialog;
        std::vector<std::shared_ptr<compression::CompressionTask>> m_active_tasks;
        bool m_is_deleting{false};
    };

} // namespace horizon::arkfm