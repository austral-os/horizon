#include <views/DesktopView.hpp>

namespace horizon::preferences
{
    DesktopView::DesktopView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        // Header Label
        auto title = std::make_unique<Label>("Fondo de Escritorio");
        title->set_fixed_size(30);
        // title->set_font_size(18); // Check if set_font_size exists
        m_title_label = title.get();
        add_child(std::move(title));

        // TODO: Implement wallpaper selection
    }

    void DesktopView::update_layout()
    {
        // Custom layout logic if needed
    }
} // namespace horizon::preferences
