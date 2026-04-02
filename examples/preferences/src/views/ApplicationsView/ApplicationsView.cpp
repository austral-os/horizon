#include <views/ApplicationsView/ApplicationsView.hpp>
#include <views/ApplicationsView/MimeTypesView.hpp>
#include <views/ApplicationsView/DefaultAppsView.hpp>
#include <horizon/ScrollArea.hpp>

namespace horizon::preferences
{
    ApplicationsView::ApplicationsView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(0);
        set_spacing(0);

        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();
        m_notebook->set_position_type(WidgetPositionTypes::FILL);

        // Inicio Tab
        auto inicio_label = std::make_unique<Label>("Inicio");
        inicio_label->set_position_type(WidgetPositionTypes::FILL);
        m_notebook->add_tab(NotebookPage("Inicio", std::move(inicio_label)));

        // Predeterminadas Tab
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_position_type(WidgetPositionTypes::FILL);
        container->set_margin(20);

        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(WidgetPositionTypes::FILL);
        
        auto default_apps_view = std::make_unique<DefaultAppsView>();
        default_apps_view->set_position_type(WidgetPositionTypes::FREE);
        
        scroll_area->set_content(std::move(default_apps_view));
        container->add_child(std::move(scroll_area));
        m_notebook->add_tab(NotebookPage("Predeterminadas", std::move(container)));

        // MIME Types Tab
        auto mime_types_view = std::make_unique<MimeTypesView>();
        m_notebook->add_tab(NotebookPage("MIME Types", std::move(mime_types_view)));

        add_child(std::move(notebook));
    }
} // namespace horizon::preferences
