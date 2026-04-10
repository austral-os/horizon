# Horizon Clipboard System Integration

This document explains how to implement clipboard support (Copy, Cut, Paste) in your Horizon widgets using the standardized signal-based and action-based API.

## 1. Overview

The Horizon clipboard system uses a **signal-driven**, **action-based**, and **lazy-delivery** model.
- **Signal-Driven**: Interactions (Global Menus, Context Menus, IPC) emit standardized signals (`"copy"`, `"cut"`, `"paste"`) which are handled by the framework.
- **Action-Based**: Widgets respond to these signals via the `perform(ClipboardAction)` method.
- **Lazy-Delivery**: Data is only transferred when requested by the destination, following the Wayland protocol model.
- **Targeting**: When a context menu is opened, the framework automatically focuses the target widget to ensure signals are routed correctly.

## 2. Implementing Clipboard Support in a Widget

To make your widget clipboard-aware, you need to override several methods from the `Widget` class.

### 2.1. Basic Requirements

Override `supports_clipboard()` to return `true`. This informs the framework that it should inject clipboard menus when this widget is focused or right-clicked.

```cpp
bool supports_clipboard() const override { return true; }
```

### 2.2. Validating Actions

Override `can_perform(ClipboardAction action)` to indicate which actions are currently available. The framework uses this to enable/disable menu items.

```cpp
bool can_perform(ClipboardAction action) const override {
    switch (action) {
        case ClipboardAction::Copy:
        case ClipboardAction::Cut:
            return has_selection(); // Replace with your selection logic
        case ClipboardAction::Paste:
            return true; // Usually widgets can always attempt to paste
    }
    return false;
}
```

### 2.3. Performing Actions

Override `perform(ClipboardAction action)` to handle the logic. This is the **internal entry point** triggered by both keyboard shortcuts and signals.

```cpp
void perform(ClipboardAction action) override {
    switch (action) {
        case ClipboardAction::Copy:
            // Notify the system that we now own the clipboard
            application()->set_clipboard_owner(this);
            break;
        case ClipboardAction::Paste:
            // Request data from the system
            application()->request_clipboard_data(this);
            break;
        case ClipboardAction::Cut:
            // Combine Copy + Delete logic
            copy_selection_to_buffer();
            application()->set_clipboard_owner(this);
            delete_selection();
            break;
    }
}
```

## 3. The Signal Protocol

The standardization relies on reserved signal IDs. When creating menu items for clipboard:
- Use IDs: `"copy"`, `"cut"`, `"paste"`.
- **CRITICAL**: Do NOT add manual `when_click` listeners to these items if they are created for a context menu via the framework injection, as the framework already provides global handlers for these IDs.

## 4. Minimal Implementation Example (Multi-Type Support)

```cpp
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <iostream>

class MyCustomWidget : public horizon::Widget {
public:
    MyCustomWidget() {
        set_focusable(true); // Required for signal targeting
    }

    bool supports_clipboard() const override { return true; }

    bool can_perform(horizon::ClipboardAction action) const override {
        return true; // Always allow for this example
    }

    void perform(horizon::ClipboardAction action) override {
        if (action == horizon::ClipboardAction::Copy) {
            application()->set_clipboard_owner(this);
            std::cout << "Widget: Offering multiple formats (plain + html)" << std::endl;
        } else if (action == horizon::ClipboardAction::Paste) {
            // The framework will negotiate the best mime type from accepted_mime_types()
            application()->request_clipboard_data(this);
        }
    }

    // --- Offering multiple formats (Lazy Delivery) ---
    void provide_clipboard_data(const std::string &mime, horizon::DataSink &sink) override {
        if (mime == "text/plain") {
            std::string data = "Standard text from MyCustomWidget";
            sink.write(std::vector<uint8_t>(data.begin(), data.end()));
            sink.done();
        } else if (mime == "text/html") {
            std::string data = "<b>Rich text</b> from <i>MyCustomWidget</i>";
            sink.write(std::vector<uint8_t>(data.begin(), data.end()));
            sink.done();
        } else {
            sink.error();
        }
    }

    // --- Consuming multiple formats ---
    void on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data) override {
        std::string content(data.begin(), data.end());
        if (mime == "text/html") {
            std::cout << "Widget Received HTML: " << content << std::endl;
        } else {
            std::cout << "Widget Received Plain Text: " << content << std::endl;
        }
    }

    // Inform the framework of our capabilities
    std::vector<std::string> provided_mime_types() const override { 
        return {"text/html", "text/plain"}; 
    }
    
    std::vector<std::string> accepted_mime_types() const override { 
        return {"text/html", "text/plain"}; 
    }
};

int main(int argc, char **argv) {
    horizon::Application app("com.example.multi_clipboard", 400, 300);
    auto widget = std::make_unique<MyCustomWidget>();
    app.set_root(std::move(widget));
    app.run();
    return 0;
}
```

## 5. Manual Triggers via Signals

If you need to trigger these actions from your own logic (e.g., from a custom button), the correct way is to emit the signal directly through the window's signal manager. This ensures the action goes through the standard pipeline (focusing the right widget, notifying the system, etc.).

```cpp
// Example: Trigger Paste from a button
my_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("paste");
});
```

Supported signal IDs for clipboard are:
- `"copy"`
- `"cut"`
- `"paste"`

## 6. UI Integration

The `WaylandWindow` automatically handles:
- **Focus Management**: Right-clicking a widget focuses it before showing the menu.
- **Global Menu**: An "Edición" menu is added to the system top bar.
- **Shortcuts**: `Ctrl+C`, `Ctrl+X`, and `Ctrl+V` emit the standard signals.
- **Context Menus**: Standard actions are injected at the **top** of the menu (followed by a separator if needed).

---
*Horizon Toolkit - Standardized Selection Management*
