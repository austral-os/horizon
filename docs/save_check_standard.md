# Horizon Save-Check System

This document explains how to implement and use the Save-Check standard in your Horizon applications and widgets. The system prevents data loss by intercepting close operations when a widget has unsaved content.

## 1. Overview

The Save-Check system follows Horizon's declarative capability model — the same pattern used by `supports_undo()`, `supports_zoom()`, `supports_printing()`, and `supports_fullscreen()`.

- **Declarative**: Widgets declare they track content modifications via `supports_save_check()`.
- **Automatic detection**: `WaylandWindow` automatically wires `when_close` to check the entire widget tree. If any descendant declares save_check and has unsaved changes, a confirmation dialog is shown before closing.
- **Per-tab granularity**: `TabCollection` consumers can check individual tab bodies before closing a specific tab.
- **Non-render flag**: The content-modified state (`m_content_modified`) is **independent** from the render-dirty flag (`m_dirty`). Modifying content does NOT automatically trigger a repaint unless the widget also calls `invalidate()`.

## 2. Architecture

```
Widget (base class)
  ├── m_content_modified (bool)  ← content state, NOT render state
  ├── supports_save_check()      ← virtual, returns false by default
  ├── is_content_modified()      ← virtual, reads m_content_modified by default
  ├── set_content_modified(bool) ← virtual, sets the flag
  └── clear_content_modified()   ← convenience, calls set_content_modified(false)

WaylandWindow
  ├── detect_save_check_support(root)         ← scans tree for save_check widgets
  ├── has_dirty_save_check_widgets(root)      ← true if any save_check widget has modifications
  ├── collect_dirty_save_check_widgets(root)  ← gathers all dirty save_check widgets
  └── when_close handler                      ← auto-wired to show confirmation
```

### Close Flow with Save-Check

```
User clicks ✕ / Ctrl+Q / Compositor close
  │
  ▼
WaylandWindow::on_close()
  │
  ├── when_close.run(ev)
  │     │
  │     ├── [Save-Check handler]
  │     │     ├── detect_save_check_support(m_root)
  │     │     ├── has_dirty_save_check_widgets(m_root)
  │     │     └── if dirty → confirm dialog
  │     │           ├── "Close anyway"  → continue
  │     │           └── "Cancel"        → ev.stop_propagation = true
  │     │
  │     └── other handlers...
  │
  └── if !ev.stop_propagation → quit()
```

## 3. Enabling Save-Check in a Widget

### 3.1. Simple Case: Widget Manages Its Own State

Override `supports_save_check()` and use the built-in `m_content_modified` flag:

```cpp
// MyEditorWidget.hpp
class MyEditorWidget : public Widget {
public:
    // ...
    bool supports_save_check() const override { return true; }

    // Call this when content changes:
    void on_content_changed() {
        set_content_modified(true);
        // invalidate() if visual update needed (separate from save flag)
        invalidate();
    }

    // Call this after saving:
    void on_content_saved() {
        clear_content_modified();
    }
};
```

### 3.2. Delegating to an External Model

If your widget delegates data to an external model (like `TextEditorWidget` → `TextDocument`), override `is_content_modified()` to read from the model:

```cpp
// MyEditorWidget.hpp
bool supports_save_check() const override { return true; }
bool is_content_modified() const override;

// MyEditorWidget.cpp
bool MyEditorWidget::is_content_modified() const {
    return m_model ? m_model->is_dirty() : Widget::is_content_modified();
}
```

The model manages its own dirty flag independently:

```cpp
// In the model:
void insert_text(...) {
    m_is_dirty = true;
    if (on_changed) on_changed();
}
void save_to_file(...) {
    // ... write to disk ...
    m_is_dirty = false;
}
```

### 3.3. Full Custom Widget Example

Below is a complete, minimal custom widget that implements the Save-Check standard:

```cpp
// RichTextWidget.hpp
#pragma once
#include <horizon/Widget.hpp>
#include <string>

class RichTextWidget : public Widget {
public:
    RichTextWidget() {
        set_focusable(true);
        set_content_modified(false);
    }

    // --- Capability declarations ---
    bool supports_save_check() const override { return true; }

    // --- Public API ---
    void set_text(const std::string &text) {
        m_text = text;
        m_saved_text = text;
        clear_content_modified();
        invalidate();
    }

    std::string text() const { return m_text; }

    void insert(const std::string &chunk) {
        m_text += chunk;
        set_content_modified(true);
        invalidate();
    }

    void save(const std::string &path) {
        // ... write m_text to file ...
        m_saved_text = m_text;
        clear_content_modified();
    }

    bool has_unsaved_changes() const {
        return is_content_modified();
    }

protected:
    void draw(GraphicsContext &ctx) override {
        // ... render m_text ...
    }

    void handle_key_event(KeyEventContext &ev) {
        // On any text input, mark as modified
        set_content_modified(true);
        invalidate();
    }

private:
    std::string m_text;
    std::string m_saved_text; // snapshot of last saved state
};
```

**Using the widget in an application:**

```cpp
class MyApp : public ApplicationWindow {
public:
    MyApp() : ApplicationWindow("My App") {
        auto editor = std::make_unique<RichTextWidget>();
        m_editor = editor.get();
        set_content(std::move(editor));
    }

    // The WaylandWindow::when_close handler automatically shows
    // the "Unsaved Changes" confirmation if m_editor is dirty.
    // No extra code needed for window-close protection.

private:
    RichTextWidget *m_editor;
};
```

## 4. Tab-Level Save-Check

When using `TabCollection`, each tab's body widget tree can be checked independently. Connect to `when_tab_close_requested`:

```cpp
m_tabs->when_tab_close_requested.connect([this](int index) {
    Widget *body = m_tabs->tab_body(index);
    if (body && application()->has_dirty_save_check_widgets(body)) {
        bool should_close = confirm(
            i18n().tr("core.save_check.unsaved_message"),
            i18n().tr("core.save_check.unsaved_title"));
        if (!should_close) return;
    }
    m_tabs->remove_tab(index);
});
```

This checks **only** the widget subtree of the tab being closed, leaving other tabs untouched.

## 5. API Reference

### Widget (base class)

| Method | Returns | Description |
|--------|---------|-------------|
| `supports_save_check()` | `bool` (virtual) | Override to return `true` if widget tracks save state |
| `is_content_modified()` | `bool` (virtual) | Returns `m_content_modified` by default |
| `set_content_modified(bool)` | `void` (virtual) | Sets the content-modified flag |
| `clear_content_modified()` | `void` | Convenience — calls `set_content_modified(false)` |
| `m_content_modified` | `bool` (protected) | The backing flag, independent of `m_dirty` |

### WaylandWindow (detection)

| Method | Returns | Description |
|--------|---------|-------------|
| `detect_save_check_support(Widget *root)` | `bool` | Scans the tree for any widget with `supports_save_check() == true` |
| `has_dirty_save_check_widgets(Widget *root)` | `bool` | Returns `true` if any save_check widget in the tree has `is_content_modified() == true` |
| `collect_dirty_save_check_widgets(Widget *root, vector<Widget*> &out)` | `void` | Collects all dirty save_check widgets into `out` |

## 6. Important Considerations

- **`m_content_modified` is NOT `m_dirty`**: The render-dirty flag (`m_dirty`) tells the compositor to repaint. The content-modified flag (`m_content_modified`) tracks whether the user's data needs saving. They are completely independent.
- **When content changes, call BOTH** `set_content_modified(true)` **and** `invalidate()` if the visual representation needs updating.
- **After saving, call** `clear_content_modified()`. The document model should also reset its own dirty flag.
- **The auto `when_close` handler** only activates when `detect_save_check_support(m_root)` returns `true`. If no widget in the tree uses save_check, the close proceeds without interruption.
