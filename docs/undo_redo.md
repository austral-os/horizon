# Horizon Undo and Redo System

This document explains how to implement and utilize standardized undo and redo support in your Horizon applications and widgets.

## 1. Overview

Horizon's undo and redo system follows a declarative and automated model similar to clipboard and fullscreen integration:
- **Declarative**: Widgets optionally declare their ability to handle undo and redo actions.
- **Signal-Based**: Global interactions use the `"undo"` and `"redo"` signals to trigger the state change.
- **Automated Menu Management**: The Window system automatically manages the "Edit" menu. If any widget supports undo/redo, the "Undo" and "Redo" options are automatically injected at the top of the "Edit" menu, properly separated from the Clipboard actions.
- **Automatic Shortcuts**: Horizon automatically intercepts `Ctrl+Z` (Undo) and `Ctrl+Shift+Z` or `Ctrl+Y` (Redo) and routes them to the correct widget.

## 2. Enabling Undo/Redo Support in a Widget

To make your widget eligible for undo and redo, override the support method and connect to the event managers.

### 2.1. Declaration

Override `supports_undo()` in your `Widget` subclass:

```cpp
bool supports_undo() const override { 
    return true; 
}
```

*Note: Returning `true` implies support for both undo and redo operations.*

### 2.2. Handling Events

When the user triggers an undo or redo action, the framework will emit the event to the appropriate widget. You should connect to these events to update your internal state (e.g., text document history).

```cpp
when_undo.connect([this](horizon::EventContext &ctx) {
    LOG_INFO << "Performing Undo action";
    // Roll back the last change
});

when_redo.connect([this](horizon::EventContext &ctx) {
    LOG_INFO << "Performing Redo action";
    // Re-apply the last undone change
});
```

## 3. The Undo/Redo Standard

### 3.1. Global Signals
The system listens to the `"undo"` and `"redo"` signals. Emitting these signals from anywhere in the application will automatically route the request to the best candidate.

### 3.2. Menu Generation
Horizon enforces a consistent UX order for menus. If `supports_undo()` is detected in the widget tree:
1. The system creates the "Edit" menu if it doesn't exist.
2. **Undo** and **Redo** are added at the top.
3. A separator is added.
4. **Clipboard** actions (Cut, Copy, Paste) are added below if supported.

## 4. Minimal Implementation Example

```cpp
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/text/TextEditorWidget.hpp>

class MyTextEditor : public horizon::TextEditorWidget {
public:
    MyTextEditor() {
        set_focusable(true);
        
        when_undo.connect([this](horizon::EventContext &ctx) {
            document()->undo();
        });
        
        when_redo.connect([this](horizon::EventContext &ctx) {
            document()->redo();
        });
    }

    // Declare support for standard undo/redo actions
    bool supports_undo() const override { return true; }
};

int main(int argc, char **argv) {
    horizon::Application app("com.example.editor", 800, 600);
    app.set_name("Undo/Redo Example");
    
    auto editor = std::make_unique<MyTextEditor>();
    app.set_root(std::move(editor));
    
    app.run();
    return 0;
}
```

## 5. Manual Triggers via Signals

You can trigger the undo or redo action programmatically by emitting the corresponding signal to the Window's signal manager. The framework will automatically find the focused widget and dispatch the event.

```cpp
// Example: Custom button to trigger Undo
my_undo_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("undo");
});

// Example: Custom button to trigger Redo
my_redo_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("redo");
});
```

## 6. Target Discovery Logic

When an undo or redo request is initiated (either via a keyboard shortcut, a menu click, or a manual signal):
1. **Target Selection**: The system searches bottom-up starting from the **currently focused widget**. 
2. **Fallback**: If the focused widget does not support undo/redo, the system performs a top-down search from the root widget to find the first candidate that returns `true` for `supports_undo()`.
3. **Dispatch**: The event is dispatched to the located target via `when_undo` or `when_redo`.

---
*Horizon Toolkit - Standardized User Interface Management*
