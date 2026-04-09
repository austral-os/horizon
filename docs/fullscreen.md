# Horizon Fullscreen System

This document explains how to implement and utilize standardized fullscreen support in your Horizon applications and widgets.

## 1. Overview

Horizon's fullscreen system follows a declarative and automated model:
- **Declarative**: Widgets optionally declare their ability to take over the screen.
- **Automated Menu Management**: The Window system automatically manages the "Visualización" (View) menu based on the widget tree's capabilities.
- **Visual Isolation**: Entering fullscreen automatically isolates the target widget, hiding distractions like siblings, titlebars, and other non-essential UI elements.
- **Smart Targeting**: Fullscreen can be triggered globally (via F11 or the menu) and the system will intelligently find the best candidate widget to fulfill the request.

## 2. Enabling Fullscreen Support in a Widget

To make your widget eligible for fullscreen, you only need to override one method and optionally connect to transition events.

### 2.1. Declaration

Override `supports_fullscreen()` in your `Widget` subclass:

```cpp
bool supports_fullscreen() const override { 
    return true; 
}
```

### 2.2. Handling Transitions

When a widget enters or leaves fullscreen, it receives events containing the final dimensions of the window. This allows the widget to adjust its layout or internal state (e.g., resizing a video buffer or a terminal PTY).

In your widget constructor:

```cpp
when_enter_fullscreen.connect([this](FullscreenEventContext &ctx) {
    LOG_INFO << "Entering fullscreen: " << ctx.width << "x" << ctx.height;
    // Adjust internal state if necessary
});

when_leave_fullscreen.connect([this](FullscreenEventContext &ctx) {
    LOG_INFO << "Leaving fullscreen: " << ctx.width << "x" << ctx.height;
    // Restore normal state
});
```

## 3. Window System Behavior

The `WaylandWindow` class manages the lifecycle of the fullscreen transition.

### 3.1. Automatic Menu Integration

If at least one widget in the application's tree (starting from the root) returns `true` for `supports_fullscreen()`, the system will:
1. Ensure a **"Visualización"** (View) menu exists in the global menu bar.
2. Add a **"Pantalla completa"** (F11) item to that menu.
3. Automatically handle the activation of this item.

### 3.2. Transition Logic (The Isolation Protocol)

When a fullscreen request is initiated:
1. **Target Selection**: The system looks for the current focused widget. If it doesn't support fullscreen, it performs a depth-first search to find the first widget in the tree that does.
2. **Isolation**: Every widget that is not a direct ancestor or a descendant of the target is hidden (`set_visible(false)`).
3. **Decoration Hiding**: The window's `Titlebar` is automatically hidden.
4. **Compositor Request**: The window sends a request to the Wayland compositor to enter the fullscreen state.
5. **Event Dispatch**: The `when_enter_fullscreen` event is fired on the target widget.

Upon exit:
- All hidden widgets are restored to their original visibility state.
- The `Titlebar` is restored.
- The `when_leave_fullscreen` event is fired.

## 4. Best Practices

- **Layout Fluidity**: Since fullscreen usually changes the aspect ratio and size of your widget significantly, ensure yours uses a responsive layout strategy (e.g., `calculate_layout` should handle all sizing).
- **Focus Management**: Focus helps the system identify *which* widget should go fullscreen if there are multiple candidates. Ensure your primary interactive widgets use `set_focusable(true)`.
- **Shortcut Handling**: Standard `F11` is reserved for the global fullscreen toggle and should not be overridden for other purposes in fullscreen-capable widgets.

---
*Horizon Toolkit - Standardized User Interface Management*
