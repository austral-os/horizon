# Drag-and-Drop Standard in Horizon

This document describes how to implement Drag-and-Drop (DnD) functionality within the Horizon ecosystem. The system is fully compatible with external applications via the Wayland protocol and optimized for high-performance internal transfers.

## Core Concepts

Drag-and-Drop in Horizon is divided into two roles:

1.  **Source**: The widget that initiates the drag (e.g., a file icon).
2.  **Target**: The widget that accepts elements dropped onto it (e.g., a folder or an editor).

---

## 1. Implementing the Source (Drag)

To make a widget draggable, you must set its `draggable` property and connect to the `when_drag_start` event.

### Steps:
1.  Call `set_draggable(true)`.
2.  Connect the `when_drag_start` event.
3.  Inside the event handler, call `application()->start_drag()`.

```cpp
// Example: A widget that drags plain text
myWidget->set_draggable(true);
myWidget->when_drag_start.connect([this](DragEventContext &ctx) {
    std::vector<std::string> mimes = {"text/plain"};
    
    // The fetcher is a lambda that executes when the receiver requests data
    auto fetcher = [](const std::string &mime) -> std::vector<uint8_t> {
        std::string data = "Hello from Horizon!";
        return std::vector<uint8_t>(data.begin(), data.end());
    };

    // Initiate the global drag operation
    application()->start_drag(mimes, fetcher, this);
});
```

---

## 2. Implementing the Target (Drop)

To allow a widget to receive drops, you must enable drop acceptance and handle the `when_drop` event.

### Steps:
1.  Call `set_accept_drops(true)`.
2.  Connect the `when_drop` event.
3.  Use `ctx.get_data(mime)` to retrieve the information.

```cpp
// Example: A widget that receives text
targetWidget->set_accept_drops(true);
targetWidget->when_drop.connect([](DropEventContext &ctx) {
    // Request data in the desired format
    auto data = ctx.get_data("text/plain");
    
    if (!data.empty()) {
        std::string text(data.begin(), data.end());
        LOG_INFO << "Received: " << text;
    }
});
```

---

## Minimalist Full Example

Here is a complete example of a window with a "Sender" and a "Receiver":

```cpp
#include <horizon/WaylandWindow.hpp>
#include <horizon/Label.hpp>

using namespace horizon;

void setup_dnd_demo(WaylandWindow &window) {
    auto container = std::make_unique<Widget>();
    
    // --- SENDER SIDE ---
    auto source = container->add_child<Label>("Drag Me");
    source->set_draggable(true);
    source->when_drag_start.connect([source](DragEventContext &ctx) {
        application()->start_drag(
            {"text/plain"}, 
            [](const std::string &m) { 
                return std::vector<uint8_t>{'O', 'K'}; 
            }, 
            source
        );
    });

    // --- RECEIVER SIDE ---
    auto target = container->add_child<Label>("Drop Here");
    target->set_accept_drops(true);
    target->when_drop.connect([target](DropEventContext &ctx) {
        auto raw = ctx.get_data("text/plain");
        target->set_text("Received!");
    });

    window.set_root(std::move(container));
}
```

## Advanced Tips

*   **Internal Bypass**: Horizon automatically detects if the drag operation is happening within the same process. It bypasses Wayland pipes and uses memory direct-access for maximum performance and stability.
*   **Formats**: For files, use the `text/uri-list` MIME type. Data will be provided as a list of URIs (paths) separated by newlines.
*   **Visual Feedback**: You can use `when_drag_enter` and `when_drag_leave` on the target to change widget colors or styles, providing immediate visual feedback during the drag operation.
