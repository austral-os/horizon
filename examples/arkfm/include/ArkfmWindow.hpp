#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/MessageDialog.hpp>

namespace horizon
{
    class Label;
    class ProgressBar;
}

namespace horizon::arkfm
{
    class ArkfmView;


    class ArkfmWindow : public ApplicationWindow
    {
    public:
        ArkfmWindow(int w, int h);
        ~ArkfmWindow() override = default;

        bool has_clipboard_content() const { return !m_clipboard_path.empty(); }
        void handle_copy(const std::string &path);
        void handle_cut(const std::string &path);
        void handle_paste(const std::string &target_dir);
        void handle_rename(const std::string &path);
        void handle_delete(const std::string &path);

        void alert(const std::string &message, const std::string &title = "Alert", horizon::MessageType type = horizon::MessageType::Info);
        bool confirm(const std::string &message, const std::string &title = "Confirm");

    private:
        void show_status_message(const std::string &msg, int timeout_ms = 5000);

        std::string m_clipboard_path;
        bool m_is_cut{false};

        horizon::Label *m_status_label{nullptr};
        horizon::ProgressBar *m_progress_bar{nullptr};
        ArkfmView *m_view_ptr{nullptr};

    };

} // namespace horizon::arkfm