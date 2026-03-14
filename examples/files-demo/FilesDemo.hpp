#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/TableView.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/arkutils/FileSystemModel.hpp>
#include <memory>
#include <string>
#include <vector>

namespace horizon::demo
{
    class FilesDemo
    {
    public:
        FilesDemo();
        int run();

    private:
        void refresh_ui(const std::string &path);
        void update_table(const std::vector<arkutils::FileInfo> &files);

        std::unique_ptr<WaylandWindow> m_app;
        ApplicationWindow *m_window = nullptr;
        TableView<arkutils::FileInfo> *m_table = nullptr;
        CoverFlow<arkutils::FileInfo> *m_coverflow = nullptr;
        Widget *m_coverflow_container = nullptr;
        Widget *m_view_container = nullptr;
        bool m_is_coverflow_view{false};
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;

        std::string m_current_path;
    };
} // namespace horizon::demo
