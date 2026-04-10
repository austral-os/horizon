# Horizon Fullscreen System

This document explains how to implement and utilize standardized fullscreen support in your Horizon applications and widgets.

## 1. Overview

Horizon's fullscreen system follows a declarative and automated model:
- **Declarative**: Widgets optionally declare their ability to take over the screen.
- **Signal-Based**: Global interactions use the `"fullscreen"` signal to toggle state.
- **Automated Menu Management**: The Window system automatically manages the "Visualización" (View) menu.
- **Visual Isolation**: Entering fullscreen isolates the target widget, hiding siblings, titlebars, and other UI distractions.
- **Menu Ordering**: In context menus, Fullscreen actions always appear **after** Clipboard actions, separated by a line.

## 2. Enabling Fullscreen Support in a Widget

To make your widget eligible for fullscreen, override the support method and handle dimensions.

### 2.1. Declaration

Override `supports_fullscreen()` in your `Widget` subclass:

```cpp
bool supports_fullscreen() const override { 
    return true; 
}
```

### 2.2. Handling Transitions

When a widget enters or leaves fullscreen, it should respond to size changes to adapt its content.

```cpp
when_enter_fullscreen.connect([this](FullscreenEventContext &ctx) {
    LOG_INFO << "Resizing content for fullscreen: " << ctx.width << "x" << ctx.height;
    // Update internal geometry or engine state
});
```

## 3. The Fullscreen Standard

### 3.1. Global Signal
The system listens to the `"fullscreen"` signal. Items with the ID `"fullscreen"` in the global menu trigger this behavior.

### 3.2. Context Menu Ordering
Horizon enforces a consistent UX order for context menus:
1.  **Clipboard Section** (Cut, Copy, Paste).
2.  (Separator)
3.  **Fullscreen Section** (Toggle Fullscreen).
4.  (Separator)
5.  **Widget-Specific Actions**.

The framework automatically handles this ordering in `show_context_menu`.

## 4. Minimal Implementation Example

```cpp
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/ColorArea2D.hpp>

class MyFullscreenWidget : public horizon::ColorArea2D {
public:
    MyFullscreenWidget() : ColorArea2D({0.2f, 0.4f, 0.8f, 1.0f}) {
        set_focusable(true);
        
        when_enter_fullscreen.connect([this](horizon::FullscreenEventContext &ctx) {
            set_background_color({0.8f, 0.2f, 0.2f, 1.0f}); // Change color when fullscreen
        });
        
        when_leave_fullscreen.connect([this](horizon::FullscreenEventContext &ctx) {
            set_background_color({0.2f, 0.4f, 0.8f, 1.0f}); // Restore color
        });
    }

    bool supports_fullscreen() const override { return true; }
};

int main(int argc, char **argv) {
    horizon::Application app("com.example.fullscreen", 800, 600);
    app.set_name("Fullscreen Example");
    
    auto widget = std::make_unique<MyFullscreenWidget>();
    app.set_root(std::move(widget));
    
    app.run();
    return 0;
}
```

## 5. Manual Triggers via Signals

You can force the fullscreen state change from anywhere in your code by emitting the `"fullscreen"` signal. This will automatically trigger the isolation logic and hide the titlebar.

```cpp
// Example: Custom button to toggle fullscreen
my_custom_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("fullscreen");
});
```

*Note: The "fullscreen" signal acts as a toggle (switches between states).*

## 6. Transition Logic (The Isolation Protocol)

When a fullscreen request is initiated:
1. **Target Selection**: The system uses the focused widget if it supports fullscreen. Otherwise, it finds the best candidate in the tree.
2. **Isolation**: Non-descendant widgets are hidden, and the window titlebar disappears.
3. **Dispatch**: Final dimensions are sent to the target via `when_enter_fullscreen`.

---
*Horizon Toolkit - Standardized User Interface Management*
