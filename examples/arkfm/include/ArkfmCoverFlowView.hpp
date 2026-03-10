#pragma once

#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include <memory>
#include <string>
#include <vector>

namespace horizon
{
    template <typename T> class CoverFlow;
}

namespace horizon::arkutils
{
    class FileSystemModel;
}

namespace horizon::arkfm
{

    class ArkfmListView;

    class ArkfmCoverFlowView : public Widget
    {
    public:
        ArkfmCoverFlowView(std::string path);
        ~ArkfmCoverFlowView() override;

        void refresh(const std::string &path);

    private:
        std::string m_current_path;
        horizon::CoverFlow<arkutils::FileInfo> *m_cover_flow{nullptr};
        ArkfmListView *m_list_view{nullptr};
        std::unique_ptr<horizon::arkutils::FileSystemModel> m_fs_model;

        void update_data(const std::vector<arkutils::FileInfo> &files);
    };

} // namespace horizon::arkfm
