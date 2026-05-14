#pragma once

#include "horizon/EventsManager.hpp"
#include "horizon/TableView.hpp"
#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/files/FileEvents.hpp"
#include <memory>
#include <string>
#include <vector>

namespace horizon
{
    template <typename T> class CoverFlow;
    class Label;
} // namespace horizon

namespace horizon::arkutils
{
    class FileSystemModel;
}

namespace horizon::files
{
    class FileListView;

    class FileCoverFlowView : public Widget
    {
    public:
        FileCoverFlowView(std::string path);
        ~FileCoverFlowView() override;

        void set_search_query(const std::string &query);
        void set_context_menu_factory(std::function<std::unique_ptr<horizon::Menu>(const arkutils::FileInfo &)> factory);
        void refresh(const std::string &path, const std::string &filter = "");
        void update_table(const std::vector<arkutils::FileInfo> &files);

        EventsManager<horizon::TableViewRowMouseClickContext<arkutils::FileInfo>>
            when_row_dbl_click;
        
        EventsManager<OperationProgressEvent> when_operation_progress;

        // Clipboard state
        std::vector<std::string> m_clipboard_paths;
        bool m_is_cut = false;

        std::function<std::unique_ptr<horizon::Menu>(const arkutils::FileInfo &)> m_context_menu_factory;

    private:
        std::string m_current_path;
        horizon::CoverFlow<arkutils::FileInfo> *m_cover_flow{nullptr};
        horizon::Label *m_navigation_label{nullptr};
        FileListView *m_list_view{nullptr};
        std::unique_ptr<horizon::arkutils::FileSystemModel> m_fs_model;

        void update_data(const std::vector<arkutils::FileInfo> &files);
    };

} // namespace horizon::files
