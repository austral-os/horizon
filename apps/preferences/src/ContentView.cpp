#include "ContentView.hpp"
#include <horizon/WaylandWindow.hpp>

namespace horizon::preferences
{
    ContentView::ContentView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
    }

    void ContentView::load_view(std::unique_ptr<Widget> view)
    {
        auto app = application();
        if (app)
        {
            auto shared_v = std::make_shared<std::unique_ptr<Widget>>(std::move(view));
            app->post_task([this, shared_v]() {
                clear_children();
                if (*shared_v)
                {
                    add_child(std::move(*shared_v));
                }
                invalidate();
            });
        }
        else
        {
            clear_children();
            if (view)
            {
                add_child(std::move(view));
            }
            invalidate();
        }
    }
} // namespace horizon::preferences
