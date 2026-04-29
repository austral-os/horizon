#include <horizon/Icon.hpp>
#include <horizon/UnderConstruction.hpp>

namespace horizon
{
    UnderConstruction::UnderConstruction()
    {
        // Set layout to vertical to allow future additions if needed,
        // though for now it's just a single centered icon.
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto icon = std::make_unique<Icon>();
        // Using the exact icon name requested by the user.
        icon->set_icon_name("emblem-under-construction");
        icon->set_icon_size(512); // Large size as it's the main content of the section
        icon->set_horizontal_alignment(TextAlignment::Center);
        icon->set_vertical_alignment(VerticalAlignment::Middle);

        add_child(std::move(icon));
    }

    void UnderConstruction::draw(GraphicsContext &gc)
    {
        // Draw standard widget background/border
        Widget::draw(gc);

        // Children (the icon) will be drawn by Widget::render
    }
} // namespace horizon
