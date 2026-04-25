#pragma once

#include <horizon/ApplicationWindow.hpp>

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
        void handle_new_folder();


        void show_status_message(const std::string &msg, int timeout_ms = 3000);

    private:
        files::FileView *m_view_ptr{nullptr};
        horizon::Label *m_status_label{nullptr};
        horizon::ProgressBar *m_progress_bar{nullptr};
        std::unique_ptr<horizon::Menu> m_active_context_menu;

    };

} // namespace horizon::arkfm