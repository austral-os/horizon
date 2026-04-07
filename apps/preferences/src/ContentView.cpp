#include "ContentView.hpp"

namespace horizon::preferences
{
    ContentView::ContentView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
    }

    void ContentView::load_view(std::unique_ptr<Widget> view)
    {
        clear_children();
        if (view)
        {
            add_child(std::move(view));
        }
        invalidate();
    }
} // namespace horizon::preferences
