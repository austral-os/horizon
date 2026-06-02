#pragma once

#include <horizon/Menu.hpp>
#include <horizon/arkutils/FileInfo.hpp>
#include <horizon/Application.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace horizon::files
{
    /**
     * @class FileContextMenuBuilder
     * @brief Builds standard context menus for file managers and desktop icons.
     */
    class FileContextMenuBuilder
    {
    public:
        struct Callbacks {
            std::function<void(const std::vector<std::string>&)> on_delete;
            std::function<void(const std::vector<std::string>&)> on_trash;
            std::function<void(const std::vector<std::string>&)> on_restore;
            std::function<void()> on_open_terminal;
            std::function<void()> on_connect_to_server;
            std::function<void()> on_toggle_hidden;
            std::function<void()> on_refresh;
        };

        static std::unique_ptr<Menu> build_item_menu(
            const arkutils::FileInfo &f, 
            const Callbacks &callbacks = {}
        );

        static std::unique_ptr<Menu> build_empty_space_menu(
            const std::string &current_path,
            bool show_hidden,
            const Callbacks &callbacks = {}
        );
    };
} // namespace horizon::files
