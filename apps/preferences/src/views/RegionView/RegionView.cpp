#include <views/RegionView/RegionView.hpp>
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    RegionView::RegionView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.region"));
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));
    }
}
