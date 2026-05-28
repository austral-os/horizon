#pragma once

#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include <string>
#include <vector>

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

    class FileDialogAcceptedContext : public EventContext
    {
    public:
        std::string selected_path;
    };

    class FileDialogCancelledContext : public EventContext
    {
    public:
    };

    enum class FileDialogMode
    {
        Open,
        Save,
        SaveAs,
        SelectFolder,
        New
    };

    struct FileFilter {
        std::string name;
        std::vector<std::string> patterns;
    };

    class FileDialog : public WaylandWindow
    {
    public:
        FileDialog(FileDialogMode mode, const std::string &title = "");
        ~FileDialog() override;

        void set_filters(const std::vector<FileFilter>& filters);

        void set_current_path(const std::string &path);
        std::string selected_path() const;

        EventsManager<FileDialogAcceptedContext> when_accepted;
        EventsManager<FileDialogCancelledContext> when_cancelled;

    private:
        void setup_ui();
        void handle_accept();

        FileDialogMode m_mode;
        files::FileView *m_view{nullptr};
        files::FileSidebar *m_sidebar{nullptr};
        files::FileToolbar *m_toolbar{nullptr};
        
        TextBoxBase *m_filename_input{nullptr};
        Combo *m_filter_combo{nullptr};
        std::vector<FileFilter> m_filters;
    };
} // namespace horizon
