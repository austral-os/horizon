#include "TopPanelWidget.hpp"
#include "TopPanelMenuBar.hpp"
#include "IndicatorsContainer.hpp"
#include <horizon/Panel.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Spacer.hpp>

using namespace horizon;

TopPanelWidget::TopPanelWidget(TopPanelApplication* app)
{
    set_corner_radius(CornerRadius(0));
    set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    set_spacing(0);
    
    // 1. Menu Bar (Left)
    auto menubar = std::make_unique<TopPanelMenuBar>(app);
    m_menubar = menubar.get();
    add_child(std::move(menubar));

    // 2. Spacer (Fill) - This pushes the indicators to the right
    auto spacer = Spacer();
    spacer->set_fixed_size(-1); // Fill remaining space
    add_child(std::move(spacer));

    // 3. Indicators Container (Right)
    auto indicators = std::make_unique<IndicatorsContainer>();
    m_indicators = indicators.get();
    add_child(std::move(indicators));
}

void TopPanelWidget::handle_message(const std::string& msg)
{
    m_menubar->handle_message(msg);
}
