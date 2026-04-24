# Button Widget

The `Button` widget is a generic clickable component. It is a template class that takes a "Background Object" as a parameter to define its visual style (typically `AquaObject` for the standard glassmorphism look).

## Properties

*   **`set_text(string)`**: Sets the label of the button.
*   **`set_icon(string)`**: Sets an optional icon from the system theme.
*   **`set_accent_color(WidgetAccentColor)`**: Changes the button's primary color (e.g., `Primary` for the main action, `Error` for destructive actions).
*   **`set_corner_radius(CornerRadius)`**: Customizes the rounding of each corner independently.

## Events
The most common event for a button is `when_click`, but it also supports hover and press states for custom interactions.

## Usage Example

```cpp
auto btn = std::make_unique<Button<AquaObject>>();
btn->set_text("Click Me");
btn->set_accent_color(WidgetAccentColor::Primary);
btn->set_fixed_size(32); // Fixed height

btn->when_click.connect([](MouseButtonEventContext &) {
    LOG_INFO << "Button was clicked!";
});
```

## Specialized Buttons
Horizon also provides specialized button types for specific UI contexts:
*   **`ToolbarButton`**: Optimized for toolbars, usually icon-only or with minimal text.
*   **`GroupButton`**: Designed to be used in groups with shared borders and radii.
*   **`TitlebarCircleButton`**: The small circular buttons used for window controls (close, minimize, maximize).
