#pragma once

#include "horizon/TableView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"

namespace horizon::files
{
    class FileListView : public TableView<arkutils::FileInfo>
    {
    public:
        FileListView(std::string path);
        ~FileListView() override = default;

        void refresh(const std::string &path, const std::string &filter = "");
        void update_table(const std::vector<arkutils::FileInfo> &files);
        void set_context_menu_factory(std::function<std::unique_ptr<Menu>(const arkutils::FileInfo &)> factory)
        {
            set_row_menu_factory(factory);
        }

    private:
        std::string m_current_path;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
    };

} // namespace horizon::files
