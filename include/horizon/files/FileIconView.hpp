#pragma once

#include "horizon/IconView.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
#include "horizon/files/FileEvents.hpp"
#include <memory>
#include <string>

namespace horizon::files
{
    class FileIconView : public IconView<arkutils::FileInfo>
    {
    public:
        FileIconView(std::string path);
        ~FileIconView() override;

        void set_application_recursive(WaylandWindow *app) override;
        void refresh(const std::string &path, const std::string &filter = "");
        void update_grid(const std::vector<arkutils::FileInfo> &files);
        void set_context_menu_factory(std::function<std::unique_ptr<Menu>(const arkutils::FileInfo &)> factory)
        {
            set_item_menu_factory(factory);
        }

        void set_show_hidden_files(bool show) { m_show_hidden_files = show; }

        EventsManager<OperationProgressEvent> when_operation_progress;

    private:
        void update_icons(const std::vector<arkutils::FileInfo> &files);
        void start_thumbnail_watch();
        void check_thumbnails();

        std::string m_current_path;
        bool m_show_hidden_files = false;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;
        uint64_t m_thumbnail_timer_id{0};
    };
} // namespace horizon::files
