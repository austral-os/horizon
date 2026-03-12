#include "TopPanelWidget.hpp"
#include "TopPanelMenuBar.hpp"
#include <horizon/Panel.hpp>
#include <horizon/GraphicsContext.hpp>

using namespace horizon;

TopPanelWidget::TopPanelWidget(TopPanelApplication* app)
{
    set_corner_radius(CornerRadius(0));
    
    auto menubar = std::make_unique<TopPanelMenuBar>(app);
    m_menubar = menubar.get();
    add_child(std::move(menubar));
}

void TopPanelWidget::handle_message(const std::string& msg)
{
    m_menubar->handle_message(msg);
}
