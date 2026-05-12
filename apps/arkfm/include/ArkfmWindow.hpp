#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon-remote-storage/RemoteManager.hpp>

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

namespace horizon::arkfm
{

    class ArkfmWindow : public ApplicationWindow
    {
    public:
        ArkfmWindow(int w = 1200, int h = 720);
        ~ArkfmWindow() override;

        void handle_rename(const std::string &path);
        void handle_delete(const std::string &path);
        void handle_open();
        void handle_properties();
        void handle_new_folder();
        void handle_mount_remote(const std::string &uri, storage::RemoteCredentials creds = {});


        void show_status_message(const std::string &msg, int timeout_ms = 3000);

    private:
        files::FileView *m_view_ptr{nullptr};
        files::FileSidebar *m_sidebar_ptr{nullptr};
        horizon::Label *m_status_label{nullptr};
        horizon::ProgressBar *m_progress_bar{nullptr};
        std::unique_ptr<horizon::Menu> m_active_context_menu;
        std::unique_ptr<storage::RemoteManager> m_remote_manager;

    };

} // namespace horizon::arkfm