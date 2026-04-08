# Horizon Clipboard System Integration

This document explains how to implement clipboard support (Copy, Cut, Paste) in your Horizon widgets using the standardized action-based API.

## 1. Overview

The Horizon clipboard system uses an **action-based** and **lazy-delivery** model.
- **Action-Based**: Instead of manual copy/paste methods, widgets respond to `ClipboardAction` commands.
- **Lazy-Delivery**: Data is only transferred when requested by the destination, following the Wayland protocol model.
- **State-Driven**: The system manages the selection life cycle (Local Ownership, Remote Offer, Transference) automatically.

## 2. Implementing Clipboard Support in a Widget

To make your widget clipboard-aware, you need to override several methods from the `Widget` class.

### 2.1. Basic Requirements

Override `supports_clipboard()` to return `true`.

```cpp
bool supports_clipboard() const override { return true; }
```

### 2.2. Validating Actions

Override `can_perform(ClipboardAction action)` to indicate which actions are currently available (e.g., if there is a selection).

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

Override `perform(ClipboardAction action)` to handle the logic for Copy, Cut, and Paste.

```cpp
void perform(ClipboardAction action) override {
    switch (action) {
        case ClipboardAction::Copy:
            // Notify the system that we now own the clipboard
            window()->set_clipboard_owner(this);
            break;
        case ClipboardAction::Cut:
            copy_selection_to_internal_buffer();
            window()->set_clipboard_owner(this);
            delete_selection();
            break;
        case ClipboardAction::Paste:
            // Request data from the system
            window()->request_clipboard_data(this);
            break;
    }
}
```

### 2.4. Providing Data (Copy/Cut)

If your widget becomes the clipboard owner via `set_clipboard_owner(this)`, the system will call `provide_clipboard_data` when another application (or your own) requests data.

```cpp
void provide_clipboard_data(const std::string &mime, DataSink &sink) override {
    if (mime == "text/plain") {
        std::string text = get_selected_text();
        sink.write(std::vector<uint8_t>(text.begin(), text.end()));
        sink.done();
    } else {
        sink.error();
    }
}

// You must also specify which MIME types you provide
virtual std::vector<std::string> provided_mime_types() const {
    return {"text/plain"};
}
```

### 2.5. Consuming Data (Paste)

When you call `window()->request_clipboard_data(this)`, the system negotiates MIME types and calls `on_clipboard_data_received` when the data arrives.

```cpp
void on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data) override {
    if (mime == "text/plain") {
        std::string text(data.begin(), data.end());
        insert_text_at_cursor(text);
    }
}

// Specify which MIME types your widget accepts (in preference order)
std::vector<std::string> accepted_mime_types() const override {
    return {"text/plain", "text/plain;charset=utf-8"};
}
```

## 3. UI Integration

The `WaylandWindow` automatically handles:
- **Global Menu**: An "Edición" menu is added to the system top bar with Copy, Cut, and Paste items.
- **Shortcuts**: `Ctrl+C`, `Ctrl+X`, and `Ctrl+V` are automatically dispatched to the focused widget.
- **Context Menus**: If your widget supports clipboard, standard actions are automatically injected into its right-click context menu.

## 4. Best Practices

- **Never use raw pointers**: The system uses `generation_id` to track selection age. If your widget is destroyed, the backend is notified automatically.
- **Non-blocking logic**: `provide_clipboard_data` can be called from internal protocol threads. The `sink` handles data safely. `on_clipboard_data_received` is always called from the **main UI thread**.
- **MIME Negotiation**: Always provide and accept standard MIME types (like `text/plain`) for maximum compatibility.

---
*Horizon Toolkit - Standardized Selection Management*
