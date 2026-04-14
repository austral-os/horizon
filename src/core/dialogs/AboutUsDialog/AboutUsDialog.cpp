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

    void AboutUsDialog::setup_ui()
    {
        auto window_widget = std::make_unique<Window>(i18n().tr("core.dialog.aboutus.title"));
        auto *app_window = window_widget.get();
        app_window->set_size(700, 550);

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_margin(12);
        root->set_spacing(12);

        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(50);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(m_content->icon);
        icon->set_icon_size(64);
        icon->set_vertical_alignment(VerticalAlignment::Top);
        icon->set_horizontal_alignment(TextAlignment::Center);
        icon->set_fixed_size(72);

        auto title_container = std::make_unique<Widget>();
        title_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto app_name = std::make_unique<Label>();
        app_name->set_text(m_content->title);
        app_name->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        app_name->set_font_size(24);
        app_name->set_fixed_size(40);

        auto app_version = std::make_unique<Label>();
        app_version->set_text(m_content->version);
        app_version->set_font_size(16);
        app_version->set_vertical_alignment(VerticalAlignment::Top);

        title_container->add_child(std::move(app_name));
        title_container->add_child(std::move(app_version));

        header->add_child(std::move(icon));
        header->add_child(std::move(title_container));

        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();

        root->add_child(std::move(header));
        root->add_child(std::move(notebook));

        app_window->add_child(std::move(root));

        set_root(std::move(window_widget));
    }

    void AboutUsDialog::build_tabs()
    {
        if (m_content->about != nullptr)
        {
            m_notebook->add_tab(
                NotebookPage(i18n().tr("core.dialog.aboutus.about"), std::move(m_content->about)));
            m_content->about = nullptr;
        }

        if (m_content->components != nullptr)
        {
            m_notebook->add_tab(NotebookPage(i18n().tr("core.dialog.aboutus.components"),
                                             std::move(m_content->components)));
            m_content->components = nullptr;
        }

        if (m_content->auths != nullptr)
        {
            m_notebook->add_tab(
                NotebookPage(i18n().tr("core.dialog.aboutus.auths"), std::move(m_content->auths)));
            m_content->auths = nullptr;
        }

        if (m_content->thanks != nullptr)
        {
            m_notebook->add_tab(NotebookPage(i18n().tr("core.dialog.aboutus.thanks"),
                                             std::move(m_content->thanks)));
            m_content->thanks = nullptr;
        }

        if (m_content->translate != nullptr)
        {
            m_notebook->add_tab(NotebookPage(i18n().tr("core.dialog.aboutus.translate"),
                                             std::move(m_content->translate)));
            m_content->translate = nullptr;
        }

        if (m_notebook->children().size() > 0)
        {
            m_notebook->set_current_tab(0);
        }
    }

} // namespace horizon
