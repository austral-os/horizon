#pragma once

#include "horizon/IconView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
#include <memory>
#include <string>

namespace horizon::arkfm
{
    class ArkfmIconView : public IconView<arkutils::FileInfo>
    {
    public:
        ArkfmIconView(std::string path);
        ~ArkfmIconView() override = default;

        void refresh(const std::string &path, const std::string &filter = "");

    private:
        void update_icons(const std::vector<arkutils::FileInfo> &files);

        std::string m_current_path;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
    };
} // namespace horizon::arkfm
