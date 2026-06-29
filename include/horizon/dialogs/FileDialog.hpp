#pragma once

#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/dialogs/FileFilter.hpp"
#include <string>
#include <vector>

namespace horizon::arkutils
{
    struct FileInfo;
}

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
        std::vector<std::string> selected_paths;
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

    class FileDialog : public WaylandWindow
    {
    public:
        FileDialog(FileDialogMode mode, const std::string &title = "");
        ~FileDialog() override;

        void set_filters(const std::vector<FileFilter>& filters);

        void set_current_path(const std::string &path);
        std::string selected_path() const;
        void set_select_multiple(bool select_multiple);
        bool select_multiple() const;

        EventsManager<FileDialogAcceptedContext> when_accepted;
        EventsManager<FileDialogCancelledContext> when_cancelled;

    private:
        void setup_ui();
        void handle_accept();
        void handle_item_selected(const arkutils::FileInfo &file);
        void accept_paths(const std::vector<std::string> &paths);
        std::string extension_for_selected_filter() const;

        FileDialogMode m_mode;
        files::FileView *m_view{nullptr};
        files::FileSidebar *m_sidebar{nullptr};
        files::FileToolbar *m_toolbar{nullptr};
        
        TextBoxBase *m_filename_input{nullptr};
        Combo *m_filter_combo{nullptr};
        std::vector<FileFilter> m_filters;
        bool m_select_multiple{false};
    };
} // namespace horizon
