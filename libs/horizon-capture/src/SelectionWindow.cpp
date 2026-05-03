#include <horizon/capture/SelectionWindow.h>
#include <horizon/WaylandSurface.hpp>

namespace horizon::capture {

SelectionWindow::SelectionWindow() 
    : WaylandLayerWindow("horizon.capture.selection", 3) // 3 = OVERLAY layer
{
    // Ensure we are above everything and not restricted by panels
    set_exclusive_zone(-1);
    
    // Set anchor to fill the entire screen
    set_anchor(15); // TOP | BOTTOM | LEFT | RIGHT
    
    // Allow keyboard interactivity to catch Esc
    set_keyboard_interactivity(1); // ON-DEMAND

    auto widget = std::make_unique<SelectionWidget>();
    m_selection_widget = widget.get();
    set_root(std::move(widget));
}

SelectionWindow::~SelectionWindow() = default;

} // namespace horizon::capture
