# Font Selection System

Horizon provides a unified system for font selection, consisting of the `FontDialog` (the actual selection window) and the `FontSelector` widget (the standard UI component for forms and settings).

## Key Components

### FontSelection
A simple structure that carries the information about a selected font:
- `family` (std::string): The font family name (e.g., "Inter", "Monospace").
- `style` (std::string): The style variant (e.g., "Regular", "Bold", "Italic").
- `size` (float): The font size in pixels or points.

### FontSelector Widget
The `FontSelector` is the recommended way to integrate font selection. It displays the currently selected font and provides a "Select..." button that automatically handles the lifecycle of the `FontDialog`.

## Usage Example

The following example shows how to use the `FontSelector` widget within a window to allow the user to change the font of a text label in real-time.

```cpp
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/FontSelector.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;

class FontDemoWindow : public Window {
public:
    FontDemoWindow() : Window("Font Selector Example") {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);
        set_spacing(20);

        // 1. Create a label to preview the font
        auto preview = std::make_unique<Label>("The quick brown fox jumps over the lazy dog");
        m_preview_ptr = preview.get();
        add_child(std::move(preview));

        // 2. Create the FontSelector widget
        auto selector = std::make_unique<FontSelector>();
        
        // Initial selection
        FontSelection initial;
        initial.family = "Sans";
        initial.size = 14.0f;
        selector->set_selection(initial);

        // 3. Connect to the change signal
        selector->when_font_changed.connect([this](const FontDialogAcceptedContext &ctx) {
            LOG_INFO << "New font selected: " << ctx.selection.family;
            
            // Update the preview label's font
            m_preview_ptr->set_font_family(ctx.selection.family);
            m_preview_ptr->set_font_size(ctx.selection.size);
            
            // Re-render the window
            this->invalidate();
        });

        add_child(std::move(selector));
    }

private:
    Label *m_preview_ptr{nullptr};
};

int main() {
    Application app("horizon.font_selector_demo", 500, 300);
    app.set_root(std::make_unique<FontDemoWindow>());
    return app.run();
}
```

## How it works

1.  **Direct Interaction**: When the user clicks the "Select..." button in the `FontSelector`, the widget internally spawns a `FontDialog`.
2.  **Thread Safety**: The `FontSelector` handles background thread execution for the dialog and uses `application()->post_task()` to ensure that signal emissions (`when_font_changed`) occur on the main thread.
3.  **State Management**: The widget automatically updates its own label to show the name and size of the currently selected font.

---

> [!TIP]
> `FontSelector` is the primary component used in the **Terminal** and **System Settings** for font configuration. Always prefer it over manually instantiating `FontDialog` for a consistent look and feel.
