#include <horizon/capture/SelectionWindow.h>
#include <horizon/WaylandSurface.hpp>

namespace horizon::capture {

SelectionWindow::SelectionWindow() 
    : WaylandLayerWindow("horizon.capture.selection", 3) // OVERLAY
{
    // Try to anchor ONLY to Top-Left (1 | 4 = 5) and set a fixed size.
    // This often bypasses exclusive zone resizing because the window is not 
    // "stretched" between edges.
    set_anchor(5); 
    
    set_exclusive_zone(-1);
    
    // Allow keyboard interactivity to catch Esc
    set_keyboard_interactivity(1); // ON-DEMAND

    auto widget = std::make_unique<SelectionWidget>();
    m_selection_widget = widget.get();
    set_root(std::move(widget));
}

SelectionWindow::~SelectionWindow() = default;

} // namespace horizon::capture
