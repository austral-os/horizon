#pragma once

#include "horizon/TableView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
#include "horizon/files/FileEvents.hpp"

namespace horizon::files
{
    class FileListView : public TableView<arkutils::FileInfo>
    {
    public:
        FileListView(std::string path);
        ~FileListView() override = default;

        void set_application_recursive(WaylandWindow *app) override;
        void refresh(const std::string &path, const std::string &filter = "");
        void update_table(const std::vector<arkutils::FileInfo> &files);
        void set_context_menu_factory(std::function<std::unique_ptr<Menu>(const arkutils::FileInfo &)> factory)
        {
            set_row_menu_factory(factory);
        }
        
        void set_show_hidden_files(bool show) { m_show_hidden_files = show; }
        void set_file_filter(const std::vector<std::string>& filter) { m_file_filter = filter; }
        
        EventsManager<OperationProgressEvent> when_operation_progress;

    private:
        std::string m_current_path;
        bool m_show_hidden_files = false;
        std::vector<std::string> m_file_filter;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
    };

} // namespace horizon::files
