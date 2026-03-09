#pragma once

#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/TableView.hpp>
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

        std::unique_ptr<Application> m_app;
        ApplicationWindow *m_window = nullptr;
        TableView<arkutils::FileInfo> *m_table = nullptr;
        std::unique_ptr<arkutils::FileSystemModel> m_fs_model;

        std::string m_current_path;
    };
} // namespace horizon::demo
