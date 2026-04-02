#include <views/ApplicationsView/ApplicationsView.hpp>

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

        // Inicio Tab
        auto inicio_label = std::make_unique<Label>("Inicio");
        inicio_label->set_position_type(WidgetPositionTypes::FILL);
        m_notebook->add_tab(NotebookPage("Inicio", std::move(inicio_label)));

        // Predeterminadas Tab
        auto predeterminadas_label = std::make_unique<Label>("Predeterminadas");
        predeterminadas_label->set_position_type(WidgetPositionTypes::FILL);
        m_notebook->add_tab(NotebookPage("Predeterminadas", std::move(predeterminadas_label)));

        // MIME Types Tab
        auto mime_label = std::make_unique<Label>("MIME Types");
        mime_label->set_position_type(WidgetPositionTypes::FILL);
        m_notebook->add_tab(NotebookPage("MIME Types", std::move(mime_label)));

        add_child(std::move(notebook));
    }
} // namespace horizon::preferences
