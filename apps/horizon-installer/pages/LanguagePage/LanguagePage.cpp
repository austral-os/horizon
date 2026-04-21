#include "LanguagePage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/TreeViewItem.hpp>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>

namespace horizon::installer
{
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
        std::set<std::string> seen_codes;

        bool at_least_one_source_found = false;
        for (const auto& base_path : search_paths) {
            std::vector<std::string> candidates = {
                base_path + "/apps/horizon-installer/locales/",
                base_path + "/locales/"
            };

            for (const auto& path : candidates) {
                if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                    at_least_one_source_found = true;
                    for (const auto& entry : std::filesystem::directory_iterator(path)) {
                        if (entry.path().extension() == ".json") {
                            std::string lang_code = entry.path().stem().string();
                            
                            // Ignore core_ files and duplicates
                            if (lang_code.find("core_") == 0) continue;
                            if (seen_codes.count(lang_code)) continue;

                            // Use core I18n to get the human-readable name
                            std::string lang_display = horizon::i18n().get_language_name(lang_code);

                            m_name_to_code[lang_display] = lang_code;
                            auto item = std::make_unique<TreeViewItem>("locale", lang_display);
                            m_tree->add_root_item(std::move(item));
                            seen_codes.insert(lang_code);
                        }
                    }
                }
            }
        }
    }
} // namespace horizon::installer
