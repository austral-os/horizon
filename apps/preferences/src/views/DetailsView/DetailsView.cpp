#include <horizon/I18n.hpp>
#include <horizon/Overview.hpp>
#include <views/DetailsView/DetailsView.hpp>

namespace horizon::preferences
{
    DetailsView::DetailsView() : horizon::Widget()
    {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::WidgetPositionTypes::FILL);
        set_margin(0);
        set_spacing(0);

        add_child(std::make_unique<horizon::Overview>());
    }
} // namespace horizon::preferences
