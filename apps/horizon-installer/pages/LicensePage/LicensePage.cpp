#include "LicensePage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/Textarea.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/ToolbarButton.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace horizon::installer
{
    LicensePage::LicensePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.license.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.license.desc"));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer(10));

        std::string license_content = "License file not found.";
        auto search_paths = I18n::get_search_paths();
        for (const auto& base_path : search_paths) {
            std::string path = base_path + "/locales/license_lgplv3.txt";
            if (std::filesystem::exists(path)) {
                std::ifstream file(path);
                if (file.is_open()) {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    license_content = ss.str();
                    break;
                }
            }
        }

        auto license_text = std::make_unique<Textarea>();
        license_text->set_text(license_content);
        license_text->set_enabled(false);
        license_text->set_size(850, 5000);
        
        auto scroll = std::make_unique<ScrollArea>();
        scroll->set_content(std::move(license_text));
        scroll->set_fixed_size(400);
        
        add_child(std::move(scroll));

        add_child(Spacer());

        auto btn_disagree = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.disagree"), "edit-undo");
        btn_disagree->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_disagree.run(ctx); 
        });

        auto btn_agree = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.agree"), "go-next");
        btn_agree->when_click.connect([this](auto&) { 
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
        add_child(std::move(btn_container));
    }
} // namespace horizon::installer
