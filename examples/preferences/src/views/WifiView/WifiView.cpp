#include <views/WifiView/WifiView.hpp>

namespace horizon::preferences
{
    WifiView::WifiView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        auto title = std::make_unique<Label>("Wi-Fi");
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));
    }
}
