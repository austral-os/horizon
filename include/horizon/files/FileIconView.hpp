#pragma once

#include "horizon/IconView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
#include <memory>
#include <string>

namespace horizon::files
{
    class FileIconView : public IconView<arkutils::FileInfo>
    {
    public:
        FileIconView(std::string path);
        ~FileIconView() override = default;

        void set_application_recursive(WaylandWindow *app) override;
        void refresh(const std::string &path, const std::string &filter = "");
        void update_grid(const std::vector<arkutils::FileInfo> &files);
        void set_context_menu_factory(std::function<std::unique_ptr<Menu>(const arkutils::FileInfo &)> factory)
        {
            set_item_menu_factory(factory);
        }

    private:
        void update_icons(const std::vector<arkutils::FileInfo> &files);

        std::string m_current_path;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
    };
} // namespace horizon::files
