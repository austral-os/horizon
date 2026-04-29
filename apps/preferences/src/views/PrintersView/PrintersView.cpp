#include <horizon/I18n.hpp>
#include <horizon/UnderConstruction.hpp>
#include <views/PrintersView/PrintersView.hpp>

namespace horizon::preferences
{
    PrintersView::PrintersView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.printers"));
        title->set_fixed_size(30);
        m_title_label = title.get();

        auto under_construction = std::make_unique<UnderConstruction>();

        add_child(std::move(title));
        add_child(std::move(under_construction));
    }
} // namespace horizon::preferences
