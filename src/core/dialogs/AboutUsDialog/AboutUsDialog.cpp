#include "horizon/dialogs/AboutUsDialog.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Notebook.hpp"
#include "horizon/Widget.hpp"
#include <fontconfig/fontconfig.h>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TableView.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/AboutUsDialog.hpp>
#include <memory>

namespace horizon
{

    AboutUsDialog::AboutUsDialog(const std::string &title)
    {
        setup_ui(title);
    }

    void AboutUsDialog::setup_ui(const std::string &title)
    {
        auto window_widget = std::make_unique<Window>(title);
        auto *app_window = window_widget.get();
        app_window->set_size(700, 550);

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_margin(12);
        root->set_spacing(12);

        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(64);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("austral-os");
        icon->set_icon_size(64);
        icon->set_vertical_alignment(VerticalAlignment::Top);
        icon->set_horizontal_alignment(TextAlignment::Center);
        icon->set_fixed_size(72);

        auto title_container = std::make_unique<Widget>();
        title_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto app_name = std::make_unique<Label>();
        app_name->set_text("About Us");
        app_name->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        app_name->set_font_size(24);

        auto app_version = std::make_unique<Label>();
        app_version->set_text("0.1.0");
        app_version->set_font_size(16);
        app_version->set_vertical_alignment(VerticalAlignment::Top);

        title_container->add_child(std::move(app_name));
        title_container->add_child(std::move(app_version));

        header->add_child(std::move(icon));
        header->add_child(std::move(title_container));

        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();
        m_page_about = nullptr;

        root->add_child(std::move(header));
        root->add_child(std::move(notebook));

        app_window->add_child(std::move(root));

        set_root(std::move(window_widget));
    }

    void AboutUsDialog::build_tabs()
    {
        if (m_page_about != nullptr)
        {
            m_notebook->add_tab(NotebookPage("About", std::move(m_page_about)));
            m_page_about = nullptr;
        }

        if (m_page_components != nullptr)
        {
            m_notebook->add_tab(NotebookPage("Components", std::move(m_page_components)));
            m_page_components = nullptr;
        }

        if (m_page_auths != nullptr)
        {
            m_notebook->add_tab(NotebookPage("Auths", std::move(m_page_auths)));
            m_page_auths = nullptr;
        }

        if (m_page_thanks != nullptr)
        {
            m_notebook->add_tab(NotebookPage("Thanks", std::move(m_page_thanks)));
            m_page_thanks = nullptr;
        }

        if (m_page_translate != nullptr)
        {
            m_notebook->add_tab(NotebookPage("Translate", std::move(m_page_translate)));
            m_page_translate = nullptr;
        }

        if (m_notebook->children().size() > 0)
        {
            m_notebook->set_current_tab(0);
        }
    }

    void AboutUsDialog::set_about_content(std::unique_ptr<Widget> content)
    {
        m_page_about = std::move(content);
    }

    Widget *AboutUsDialog::get_about_content()
    {
        return m_page_about.get();
    }

    void AboutUsDialog::set_components_content(std::unique_ptr<Widget> content)
    {
        m_page_components = std::move(content);
    }

    Widget *AboutUsDialog::get_components_content()
    {
        return m_page_components.get();
    }

    void AboutUsDialog::set_auths_content(std::unique_ptr<Widget> content)
    {
        m_page_auths = std::move(content);
    }

    Widget *AboutUsDialog::get_auths_content()
    {
        return m_page_auths.get();
    }

    void AboutUsDialog::set_thanks_content(std::unique_ptr<Widget> content)
    {
        m_page_thanks = std::move(content);
    }

    Widget *AboutUsDialog::get_thanks_content()
    {
        return m_page_thanks.get();
    }

    void AboutUsDialog::set_translate_content(std::unique_ptr<Widget> content)
    {
        m_page_translate = std::move(content);
    }

    Widget *AboutUsDialog::get_translate_content()
    {
        return m_page_translate.get();
    }

} // namespace horizon
