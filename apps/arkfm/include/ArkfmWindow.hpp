#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/MessageDialog.hpp>

namespace horizon
{
    class Label;
    class ProgressBar;
}

namespace horizon::files
{
    class FileView;
}

namespace horizon::arkfm
{

    class ArkfmWindow : public ApplicationWindow
    {
    public:
        ArkfmWindow(int w = 1200, int h = 720);
        ~ArkfmWindow() override = default;

        void handle_rename(const std::string &path);
        void handle_delete(const std::string &path);
        void handle_open();
        void handle_properties();

        void alert(const std::string &message, const std::string &title = "Alert", horizon::MessageType type = horizon::MessageType::Info);
        bool confirm(const std::string &message, const std::string &title = "Confirm");
        void show_status_message(const std::string &msg, int timeout_ms = 3000);

    private:
        files::FileView *m_view_ptr{nullptr};
        horizon::Label *m_status_label{nullptr};
        horizon::ProgressBar *m_progress_bar{nullptr};

    };

} // namespace horizon::arkfm