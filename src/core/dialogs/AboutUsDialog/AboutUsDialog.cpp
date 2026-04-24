#include "horizon/dialogs/AboutUsDialog.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Notebook.hpp"
#include "horizon/Widget.hpp"
#include "horizon/Label.hpp"
#include "horizon/Link.hpp"
#include "horizon/ScrollArea.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/I18n.hpp"
#include "horizon/Window.hpp"
#include <memory>

namespace horizon
{
    AboutUsDialog::AboutUsDialog(AboutManager &manager) : m_manager(manager)
    {
    }

    void AboutUsDialog::setup_ui()
    {
        auto window_widget = std::make_unique<Window>(i18n().tr("core.dialog.aboutus.title"));
        auto *app_window = window_widget.get();
        app_window->set_size(600, 500);
        set_blur(true);

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_margin(16);
        root->set_spacing(16);

        // Header: Icon, Name, Version
        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(80);
        header->set_spacing(16);

        const auto &app_info = m_manager.app_data();

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(app_info.icon);
        icon->set_icon_size(64);
        icon->set_fixed_size(72);

        auto title_container = std::make_unique<Widget>();
        title_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto app_name = std::make_unique<Label>();
        app_name->set_text(app_info.title);
        app_name->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        app_name->set_font_size(24);
        app_name->set_fixed_size(32);

        auto app_version = std::make_unique<Label>();
        app_version->set_text(app_info.version);
        app_version->set_font_size(14);
        app_version->set_vertical_alignment(VerticalAlignment::Top);

        title_container->add_child(std::move(app_name));
        title_container->add_child(std::move(app_version));

        header->add_child(std::move(icon));
        header->add_child(std::move(title_container));

        auto notebook = std::make_unique<Notebook>();
        notebook->set_position_type(FILL);
        m_notebook = notebook.get();

        root->add_child(std::move(header));
        root->add_child(std::move(notebook));

        app_window->add_child(std::move(root));
        set_root(std::move(window_widget));
    }

    void AboutUsDialog::build_tabs()
    {
        // Tab 1: Application
        m_notebook->add_tab(NotebookPage(i18n().tr("core.dialog.aboutus.application"), 
                                        create_info_page(m_manager.app_data())));
        
        // Tab 2: Horizon
        m_notebook->add_tab(NotebookPage("Horizon", 
                                        create_info_page(m_manager.horizon_data())));

        if (m_notebook->children().size() > 0)
        {
            m_notebook->set_current_tab(0);
        }
    }

    std::unique_ptr<Widget> AboutUsDialog::create_info_page(const About &data)
    {
        auto scroll = std::make_unique<ScrollArea>();
        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_margin(12);
        content->set_spacing(8);

        int total_h = 24; // top + bottom margins

        if (data.title == "Horizon")
        {
            auto icon_row = std::make_unique<Widget>();
            icon_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            icon_row->set_fixed_size(140);
            
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("horizon-desktop");
            icon->set_icon_size(128);
            icon->set_fixed_size(128);
            
            icon_row->add_child(Spacer());
            icon_row->add_child(std::move(icon));
            icon_row->add_child(Spacer());
            
            content->add_child(std::move(icon_row));
            total_h += 140 + 8;

            auto title = std::make_unique<Label>(data.title);
            title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
            title->set_font_size(32);
            title->set_alignment(TextAlignment::Center);
            title->set_fixed_size(40);
            content->add_child(std::move(title));
            total_h += 40 + 8;

            auto version = std::make_unique<Label>(i18n().tr("core.dialog.aboutus.version") + " " + data.version);
            version->set_font_size(14);
            version->set_alignment(TextAlignment::Center);
            version->set_fixed_size(20);
            content->add_child(std::move(version));
            total_h += 20 + 8;

            content->add_child(Spacer(16));
            total_h += 16 + 8;
        }

        auto add_section_title = [&](const std::string &title) {
            auto lbl = std::make_unique<Label>();
            lbl->set_text(title);
            lbl->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
            lbl->set_fixed_size(24);
            content->add_child(std::move(lbl));
            total_h += 24 + 8; // height + spacing
        };

        // Description
        auto desc = std::make_unique<Label>();
        desc->set_text(data.description);
        desc->set_vertical_alignment(VerticalAlignment::Top);
        if (data.title == "Horizon") {
            desc->set_alignment(TextAlignment::Center);
        }
        desc->set_fixed_size(80); 
        content->add_child(std::move(desc));
        total_h += 80 + 8;

        // Links Section
        if (!data.web.empty() || !data.git.empty()) {
            add_section_title(i18n().tr("core.dialog.aboutus.links"));
            if (!data.web.empty()) {
                auto link = std::make_unique<Link>();
                link->set_text(data.web);
                link->set_url(data.web);
                link->set_fixed_size(20);
                content->add_child(std::move(link));
                total_h += 20 + 8;
            }
            if (!data.git.empty()) {
                auto link = std::make_unique<Link>();
                link->set_text(data.git);
                link->set_url(data.git);
                link->set_fixed_size(20);
                content->add_child(std::move(link));
                total_h += 20 + 8;
            }
            total_h += 10 + 8; // Spacer
            content->add_child(Spacer(10));
        }

        // Authors Section
        if (!data.authors.empty()) {
            add_section_title(i18n().tr("core.dialog.aboutus.authors"));
            for (const auto &author : data.authors) {
                auto lbl = std::make_unique<Label>();
                std::string text = author.name;
                if (!author.email.empty()) text += " <" + author.email + ">";
                lbl->set_text(text);
                lbl->set_fixed_size(20);
                content->add_child(std::move(lbl));
                total_h += 20 + 8;
                if (!author.url.empty()) {
                    auto link = std::make_unique<Link>();
                    link->set_text(author.url);
                    link->set_url(author.url);
                    link->set_fixed_size(18);
                    content->add_child(std::move(link));
                    total_h += 18 + 8;
                }
            }
            total_h += 10 + 8; // Spacer
            content->add_child(Spacer(10));
        }

        // Translators Section
        if (!data.translators.empty()) {
            add_section_title(i18n().tr("core.dialog.aboutus.translators"));
            for (const auto &trans : data.translators) {
                auto lbl = std::make_unique<Label>();
                lbl->set_text(trans.name);
                lbl->set_fixed_size(20);
                content->add_child(std::move(lbl));
                total_h += 20 + 8;
            }
            total_h += 10 + 8; // Spacer
            content->add_child(Spacer(10));
        }

        // Use a container widget that will hold all info and set its size based on content.
        content->set_position_type(FILL);
        content->set_height(total_h); 

        scroll->set_content(std::move(content));
        return scroll;
    }

} // namespace horizon
