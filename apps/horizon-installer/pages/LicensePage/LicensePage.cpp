#include "LicensePage.hpp"
#include <filesystem>
#include <fstream>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Textarea.hpp>
#include <horizon/ToolbarButton.hpp>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace horizon::installer
{
    LicensePage::LicensePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.license.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        title->set_fixed_size(80);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.license.desc"));
        desc->set_alignment(TextAlignment::Center);
        desc->set_fixed_size(80);
        add_child(std::move(desc));

        add_child(Spacer(10));

        std::string license_content = "License file not found.";
        std::vector<std::string> license_paths = {
            "/usr/share/horizon/license_lgplv3.txt",
        };

        // Fallback for development
        char result[4096];
        ssize_t count = readlink("/proc/self/exe", result, 4096);
        if (count != -1) {
            std::string path(result, count);
            std::string dir = path.substr(0, path.find_last_of('/'));
            license_paths.push_back(dir + "/../apps/horizon_session/data/license_lgplv3.txt");
            license_paths.push_back(dir + "/../../../apps/horizon_session/data/license_lgplv3.txt");
        }

        // Add paths from I18n searching as well
        auto i18n_paths = I18n::get_search_paths();
        for (const auto &base_path : i18n_paths)
        {
            license_paths.push_back(base_path + "/locales/license_lgplv3.txt");
            license_paths.push_back(base_path + "/license_lgplv3.txt");
        }

        for (const auto &path : license_paths)
        {
            if (std::filesystem::exists(path) && !std::filesystem::is_directory(path))
            {
                std::ifstream file(path);
                if (file.is_open())
                {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    license_content = ss.str();
                    break;
                }
            }
        }

        auto license_text = std::make_unique<Textarea>();
        license_text->set_text(license_content);
        license_text->set_fixed_size(-1);
        license_text->move_cursor_to_start();

        // license_text->set_enabled(false);
        //  license_text->set_size_size(850, 5000);

        add_child(std::move(license_text));

        add_child(Spacer(20));

        auto btn_disagree =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.disagree"), "edit-undo");
        btn_disagree->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_disagree.run(ctx);
            });

        auto btn_agree =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.agree"), "go-next");
        btn_agree->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_agree.run(ctx);
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_disagree));
        btn_container->add_child(Spacer(20));
        btn_container->add_child(std::move(btn_agree));
        btn_container->add_child(Spacer());
        btn_container->set_fixed_size(70);
        add_child(std::move(btn_container));
    }
} // namespace horizon::installer
