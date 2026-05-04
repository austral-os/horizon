#include <horizon/WaylandSurface.hpp>
#include <horizon/capture/SelectionWindow.h>

namespace horizon::capture
{

    SelectionWindow::SelectionWindow()
        : WaylandLayerWindow("horizon.capture.selection", 3, true) // OVERLAY, defer_init=true
    {
        // Anchor to all edges to cover the full screen
        set_anchor(15);

        set_exclusive_zone(-1);

        // Allow keyboard interactivity to catch Esc
        set_keyboard_interactivity(1); // ON-DEMAND

        auto widget = std::make_unique<SelectionWidget>();
        m_selection_widget = widget.get();
        set_root(std::move(widget));
    }

    SelectionWindow::~SelectionWindow() = default;

} // namespace horizon::capture
