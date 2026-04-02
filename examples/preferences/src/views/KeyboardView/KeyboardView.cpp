#include <views/KeyboardView/KeyboardView.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Label.hpp>
#include <views/KeyboardView/KeyboardHardwareView.hpp>
#include <views/KeyboardView/KeyboardLanguageView.hpp>

namespace horizon::preferences
{
    KeyboardView::KeyboardView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(0);
        set_spacing(0);

        auto notebook = std::make_unique<Notebook>();
        notebook->set_position_type(WidgetPositionTypes::FILL);
        m_notebook = notebook.get();

        // Hardware Tab
        m_notebook->add_tab(NotebookPage("Hardware", std::make_unique<KeyboardHardwareView>()));

        // Idioma Tab
        m_notebook->add_tab(NotebookPage("Idioma", std::make_unique<KeyboardLanguageView>()));

        // Atajos Tab
        auto atajos_label = std::make_unique<Label>("Atajos");
        atajos_label->set_position_type(WidgetPositionTypes::FILL);
        m_notebook->add_tab(NotebookPage("Atajos", std::move(atajos_label)));

        add_child(std::move(notebook));
    }
} // namespace horizon::preferences
