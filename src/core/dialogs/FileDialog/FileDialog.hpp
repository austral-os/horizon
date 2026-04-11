#pragma once

#include "horizon/WaylandWindow.hpp"
#include <string>
#include <vector>
#include <functional>

namespace horizon::files
{
    class FileView;
    class FileSidebar;
    class FileToolbar;
}

namespace horizon
{
    class TextBoxBase;
    class Combo;
    class Label;

    enum class FileDialogMode
    {
        Open,
        Save,
        SaveAs,
        SelectFolder,
        New
    };

    class FileDialog : public WaylandWindow
    {
    public:
        FileDialog(FileDialogMode mode, const std::string &title = "");
        ~FileDialog() override;

        void set_current_path(const std::string &path);
        std::string selected_path() const;

        std::function<void(const std::string &)> on_accepted;
        std::function<void()> on_cancelled;

    private:
        void setup_ui();
        void handle_accept();

        FileDialogMode m_mode;
        files::FileView *m_view{nullptr};
        files::FileSidebar *m_sidebar{nullptr};
        files::FileToolbar *m_toolbar{nullptr};
        
        TextBoxBase *m_filename_input{nullptr};
        Combo *m_filter_combo{nullptr};
    };
} // namespace horizon
