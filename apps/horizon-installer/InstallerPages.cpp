#include "InstallerPages.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TreeViewItem.hpp>
#include <horizon/Icon.hpp>
#include <horizon-disk-utilities/DiskManager.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <nlohmann/json.hpp>
#include <horizon/Textarea.hpp>

namespace horizon::installer
{
    // --- Helper: Circle Button styled like in the images ---
    // For now we use ToolbarButton as it is already implemented and stylized in the theme

    // --- LanguagePage ---
    LanguagePage::LanguagePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.language.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        title->set_fixed_size(60);
        add_child(std::move(title));
        add_child(Spacer(20));

        auto tree_container = std::make_unique<Widget>();
        tree_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        tree_container->add_child(Spacer());
        auto tree = std::make_unique<TreeView>();
        m_tree = tree.get();
        tree->set_size(400, 300);
        tree_container->add_child(std::move(tree));
        tree_container->add_child(Spacer());
        add_child(std::move(tree_container));
        
        load_languages();

        add_child(Spacer());

        auto btn_next = std::make_unique<ToolbarButton>("", "go-next");
        btn_next->set_fixed_size(60);
        btn_next->when_click.connect([this](auto&) {
            if (m_tree->selected_item()) {
                selected_name = m_tree->selected_item()->get_text();
                selected_code = m_name_to_code[selected_name];
                EventContext ctx;
                when_continue.run(ctx);
            }
        });
        
        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_next));
        btn_container->add_child(Spacer());
        
        add_child(std::move(btn_container));
    }

    void LanguagePage::load_languages()
    {
        m_tree->clear_root_items();
        m_name_to_code.clear();
        
        auto search_paths = I18n::get_search_paths();

        bool found = false;
        for (const auto& base_path : search_paths) {
            std::string path = base_path + "/locales/";
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.path().extension() == ".json") {
                        std::string lang_code = entry.path().stem().string();
                        // Ignore core_ files if any
                        if (lang_code.find("core_") == 0) continue;

                        std::string lang_display = lang_code;

                        // Try to parse the file to find a human-readable name
                        try {
                            std::ifstream file(entry.path());
                            if (file.is_open()) {
                                nlohmann::json data;
                                file >> data;
                                if (data.contains("language") && data["language"].contains("name")) {
                                    lang_display = data["language"]["name"].get<std::string>();
                                }
                            }
                        } catch (...) {
                            // Fallback to code if parsing fails
                        }

                        m_name_to_code[lang_display] = lang_code;
                        auto item = std::make_unique<TreeViewItem>("locale", lang_display);
                        m_tree->add_root_item(std::move(item));
                        found = true;
                    }
                }
                if (found) break;
            }
        }
        
        if (!found) {
            std::cout << "Warning: No locales found in any of the searched paths." << std::endl;
        }
    }

    // --- WelcomePage ---
    WelcomePage::WelcomePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        add_child(Spacer());

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("system-os-installer"); // Placeholder
        logo->set_icon_size(128);
        logo->set_size(128, 128);
        
        auto logo_container = std::make_unique<Widget>();
        logo_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        logo_container->add_child(Spacer());
        logo_container->add_child(std::move(logo));
        logo_container->add_child(Spacer());
        add_child(std::move(logo_container));

        auto title = std::make_unique<Label>(i18n().tr("installer.welcome.title"));
        title->set_font_size(48);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.welcome.desc"));
        desc->set_font_size(18);
        desc->set_text_color(Color(0.4f, 0.4f, 0.4f));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer());

        auto btn_next = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.continue"), "go-next");
        btn_next->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_continue.run(ctx); 
        });
        
        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_next));
        btn_container->add_child(Spacer());
        add_child(std::move(btn_container));
    }

    // --- LicensePage ---
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
                    std::cout << "Loaded license text: " << license_content.length() << " bytes from " << path << std::endl;
                    break;
                }
            }
        }

        auto license_text = std::make_unique<Textarea>();
        license_text->set_text(license_content);
        license_text->set_enabled(false); // Read-only
        license_text->set_size(850, 5000); // Increased width to fill the scroll area
        
        auto scroll = std::make_unique<ScrollArea>();
        scroll->set_content(std::move(license_text));
        scroll->set_fixed_size(400); // In vertical layout, this is the HEIGHT
        
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

    // --- DiskPage ---
    DiskPage::DiskPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.disk.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.disk.desc"));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer(20));

        auto disk_tree = std::make_unique<TreeView>();
        m_disk_tree = disk_tree.get();
        add_child(std::move(disk_tree));
        m_disk_tree->set_size(500, 250);
        
        refresh_disks();

        add_child(Spacer());

        auto btn_back = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.back"), "go-previous");
        btn_back->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_back.run(ctx); 
        });

        auto btn_install = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.install"), "system-run");
        btn_install->when_click.connect([this](auto&) {
            if (m_disk_tree->selected_item()) {
                selected_device_str = m_disk_tree->selected_item()->get_text();
                EventContext ctx;
                when_install.run(ctx);
            }
        });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_back));
        btn_container->add_child(Spacer(20));
        btn_container->add_child(std::move(btn_install));
        btn_container->add_child(Spacer());
        add_child(std::move(btn_container));
    }

    void DiskPage::refresh_disks()
    {
        m_disk_tree->clear_root_items();
        horizon::disks::DiskManager manager;
        manager.scan();
        
        for (const auto& dev : manager.devices()) {
            std::string label = dev->name + " (" + dev->human_capacity() + ")";
            auto item = std::make_unique<TreeViewItem>("drive-harddisk", label);
            m_disk_tree->add_root_item(std::move(item));
        }
    }

    // --- InstallPage ---
    InstallPage::InstallPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.install.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto progress = std::make_unique<ProgressBar>();
        m_progress = progress.get();
        add_child(std::move(progress));
        m_progress->set_size(600, 20);
        
        auto status = std::make_unique<Label>(i18n().tr("installer.install.desc"));
        m_status = status.get();
        add_child(std::move(status));
        m_status->set_alignment(TextAlignment::Center);

        add_child(Spacer());

        auto btn_cancel = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.cancel"), "process-stop");
        btn_cancel->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_cancel.run(ctx); 
        });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_cancel));
        btn_container->add_child(Spacer());
        add_child(std::move(btn_container));
    }

    void InstallPage::update_progress(float progress, const std::string& message)
    {
        m_progress->set_progress(progress);
        m_status->set_text(message);
    }

    // --- SuccessPage ---
    SuccessPage::SuccessPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        add_child(Spacer());

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("software-installed-symbolic");
        logo->set_icon_size(128);
        logo->set_size(128, 128);
        
        auto logo_container = std::make_unique<Widget>();
        logo_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        logo_container->add_child(Spacer());
        logo_container->add_child(std::move(logo));
        logo_container->add_child(Spacer());
        add_child(std::move(logo_container));

        auto title = std::make_unique<Label>(i18n().tr("installer.success.title"));
        title->set_font_size(48);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.success.desc"));
        desc->set_font_size(18);
        desc->set_text_color(Color(0.4f, 0.4f, 0.4f));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer());

        auto btn_finish = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.restart"), "system-reboot");
        btn_finish->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_finish.run(ctx); 
        });
        
        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_finish));
        btn_container->add_child(Spacer());
        add_child(std::move(btn_container));
    }

} // namespace horizon::installer
