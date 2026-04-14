# Color Selection System

Horizon provides a unified system for color selection, consisting of the `ColorPickerDialog` (the actual selection window) and the `ColorSelector` widget (the standard UI component for settings and forms).

## Key Components

### Color
The standard color representation in Horizon (`horizon::Color`). It stores RGBA values and provides utility methods like `to_hex()`.

### ColorSelector Widget
The `ColorSelector` is the recommended way to integrate color selection. It displays a preview block of the currently selected color and automatically handles the lifecycle of the `ColorPickerDialog` when clicked.

## Usage Example

The following example shows how to use the `ColorSelector` widget to allow the user to change the color of a text label. This code is based on `examples/color_picker_demo.cpp`.

```cpp
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/ColorSelector.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;

class ColorDemoWindow : public Window {
public:
    ColorDemoWindow() : Window("Demo de Selección de Color") {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);
        set_spacing(20);

        // 1. Create a label to preview the color
        auto label = std::make_unique<Label>("Horizon UI Kit - Color Picker Demo");
        m_label_ptr = label.get();
        add_child(std::move(label));

        // 2. Create the ColorSelector widget
        auto selector = std::make_unique<ColorSelector>();
        
        // Initial color selection
        Color initial_color(0.2f, 0.4f, 0.8f);
        selector->set_color(initial_color);
        m_label_ptr->set_text_color(initial_color);

        // 3. Connect to the change signal
        selector->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &ctx) {
            LOG_INFO << "Color seleccionado: " << ctx.color.to_hex();
            
            // Update the preview label's color
            m_label_ptr->set_text_color(ctx.color);
            
            // Re-render the window
            this->invalidate();
        });

        add_child(std::move(selector));
    }

private:
    Label *m_label_ptr{nullptr};
};

int main(int argc, char** argv) {
    Application app("horizon.color_demo", 500, 300);
    app.set_root(std::make_unique<ColorDemoWindow>());
    return app.run();
}
```

## How it works

1.  **Direct Interaction**: When the user clicks the `ColorSelector` widget, it inherently spans a `ColorPickerDialog`.
2.  **Thread Safety**: The `ColorSelector` manages the dialog window asynchronously and emits the `when_color_changed` event back on the main thread.
3.  **Visual Feedback**: The widget itself keeps a visual rectangle displaying the currently selected color at all times.

---

> [!TIP]
> `ColorSelector` is the primary component used in the Terminal and Configuration dialogs for selecting visual accents and customization colors.
