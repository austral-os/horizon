#include "LanguagePage.hpp"
#include <filesystem>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ToolbarButton.hpp>
#include <iostream>
#include <set>
#include <vector>

namespace horizon::installer
{
    LanguagePage::LanguagePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        // Language Icon
        auto icon_row = std::make_unique<Widget>();
        icon_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        icon_row->set_fixed_size(100);
        icon_row->add_child(Spacer());

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("preferences-desktop-locale");
        icon->set_icon_size(96);
        icon->set_width(96);
        icon_row->add_child(std::move(icon));

        icon_row->add_child(Spacer());
        add_child(std::move(icon_row));
        add_child(Spacer(20));

        auto title = std::make_unique<Label>(i18n().tr("installer.language.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        title->set_fixed_size(60);
        add_child(std::move(title));
        add_child(Spacer(20));

        auto table_container = std::make_unique<Widget>();
        table_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        table_container->add_child(Spacer(200));

        auto table = std::make_unique<TableView<LanguageItem>>();
        m_table = table.get();
        m_table->set_header_visible(false);
        m_table->set_width(400);
        m_table->set_height(350);

        TableColumn<LanguageItem> col;
        col.id = "name";
        col.title = "Name";
        col.width = -1;
        col.cell_factory = [](const LanguageItem &info)
        {
            auto lbl = std::make_unique<Label>(info.name);
            lbl->set_alignment(TextAlignment::Center);
            lbl->set_margin(10);
            return lbl;
        };
        m_table->add_column(col);

        table_container->add_child(std::move(table));
        table_container->add_child(Spacer(200));
        add_child(std::move(table_container));

        load_languages();

        add_child(Spacer(50));

        auto btn_next = std::make_unique<ToolbarButton>("Siguiente", "go-next");
        btn_next->set_fixed_size(60);
        btn_next->when_click.connect(
            [this](auto &)
            {
                int idx = m_table->selected_index();
                if (idx != -1 && (size_t)idx < m_languages.size())
                {
                    selected_name = m_languages[idx].name;
                    selected_code = m_languages[idx].code;
                    EventContext ctx;
                    when_continue.run(ctx);
                }
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->set_fixed_size(50);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_next));
        btn_container->add_child(Spacer());

        add_child(std::move(btn_container));
    }

    void LanguagePage::load_languages()
    {
        m_languages.clear();

        auto search_paths = I18n::get_search_paths();
        std::set<std::string> seen_codes;

        for (const auto &base_path : search_paths)
        {
            std::vector<std::string> candidates = {base_path + "/apps/horizon-installer/locales/",
                                                   base_path + "/locales/"};

            for (const auto &path : candidates)
            {
                if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
                {
                    for (const auto &entry : std::filesystem::directory_iterator(path))
                    {
                        if (entry.path().extension() == ".json")
                        {
                            std::string lang_code = entry.path().stem().string();

                            if (lang_code.find("core_") == 0)
                                continue;
                            if (seen_codes.count(lang_code))
                                continue;

                            std::string lang_display = horizon::i18n().get_language_name(lang_code);

                            m_languages.push_back({lang_code, lang_display});
                            seen_codes.insert(lang_code);
                        }
                    }
                }
            }
        }

        m_table->set_data(m_languages);
    }
} // namespace horizon::installer
