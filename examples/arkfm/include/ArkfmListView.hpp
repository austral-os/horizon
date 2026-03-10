#pragma once

#include "horizon/TableView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
namespace horizon::arkfm
{

    class ArkfmListView : public TableView<arkutils::FileInfo>
    {
    public:
        ArkfmListView(std::string path);
        ~ArkfmListView() override = default;

        void refresh(const std::string &path);
        void update_table(const std::vector<arkutils::FileInfo> &files);

    private:
        std::string m_current_path;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
    };

}; // namespace horizon::arkfm