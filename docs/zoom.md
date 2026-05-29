# Horizon Zoom In and Zoom Out System

This document explains how to implement and utilize standardized zoom in and zoom out support in your Horizon applications and widgets.

## 1. Overview

Horizon's zoom system follows a declarative and automated model similar to clipboard, fullscreen, and undo/redo integration:
- **Declarative**: Widgets optionally declare their ability to handle zoom actions.
- **Signal-Based**: Global interactions use the `"zoom_in"` and `"zoom_out"` signals to trigger the state change.
- **Automated Menu Management**: The Window system automatically manages the "View" (Visualización) menu. If any widget supports zoom, the "Zoom In" and "Zoom Out" options are automatically injected at the top of the "View" menu, alongside fullscreen options.
- **Automatic Shortcuts**: Horizon automatically intercepts `Ctrl++` (Zoom In) and `Ctrl+-` (Zoom Out) and routes them to the correct widget.

## 2. Enabling Zoom Support in a Widget

To make your widget eligible for zoom, override the support method and connect to the event managers.

### 2.1. Declaration

Override `supports_zoom()` in your `Widget` subclass:

```cpp
bool supports_zoom() const override { 
    return true; 
}
```

*Note: Returning `true` implies support for both zoom in and zoom out operations.*

### 2.2. Handling Events

When the user triggers a zoom action, the framework will emit the event to the appropriate widget. You should connect to these events to update your internal scale or font sizes.

```cpp
when_zoom_in.connect([this](horizon::EventContext &ctx) {
    LOG_INFO << "Performing Zoom In";
    // Increase internal scale or font size
});

when_zoom_out.connect([this](horizon::EventContext &ctx) {
    LOG_INFO << "Performing Zoom Out";
    // Decrease internal scale or font size
});
```

## 3. The Zoom Standard

### 3.1. Global Signals
The system listens to the `"zoom_in"` and `"zoom_out"` signals. Emitting these signals from anywhere in the application will automatically route the request to the best candidate.

### 3.2. Menu Generation
Horizon enforces a consistent UX order for menus. If `supports_zoom()` is detected in the widget tree:
1. The system creates the "View" menu if it doesn't exist.
2. **Zoom In** and **Zoom Out** are added at the top.
3. A separator is added.
4. **Fullscreen** action is added below if supported.

## 4. Minimal Implementation Example

```cpp
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/ImageView.hpp>

class MyZoomableImage : public horizon::ImageView {
public:
    MyZoomableImage() {
        set_focusable(true);
        
        when_zoom_in.connect([this](horizon::EventContext &ctx) {
            set_scale(get_scale() * 1.1f);
            invalidate();
        });
        
        when_zoom_out.connect([this](horizon::EventContext &ctx) {
            set_scale(get_scale() / 1.1f);
            invalidate();
        });
    }

    // Declare support for standard zoom actions
    bool supports_zoom() const override { return true; }
};
```

## 5. Manual Triggers via Signals

You can trigger the zoom actions programmatically by emitting the corresponding signal to the Window's signal manager. The framework will automatically find the focused widget and dispatch the event.

```cpp
// Example: Custom button to trigger Zoom In
my_zoom_in_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("zoom_in");
});

// Example: Custom button to trigger Zoom Out
my_zoom_out_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("zoom_out");
});
```

## 6. Target Discovery Logic

When a zoom request is initiated (either via a keyboard shortcut, a menu click, or a manual signal):
1. **Target Selection**: The system searches bottom-up starting from the **currently focused widget**. 
2. **Fallback**: If the focused widget does not support zoom, the system performs a top-down search from the root widget to find the first candidate that returns `true` for `supports_zoom()`.
3. **Dispatch**: The event is dispatched to the located target via `when_zoom_in` or `when_zoom_out`.

---
*Horizon Toolkit - Standardized User Interface Management*
