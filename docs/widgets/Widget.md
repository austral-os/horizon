# Widget Class

The `Widget` class is the foundational building block of the Horizon Desktop Environment UI. Every interactive element, from simple buttons to complex windows, inherits from this class. It provides the core mechanisms for layout, event handling, rendering, and lifecycle management.

## Core Concepts

### 1. Geometry and Layout
Horizon uses a hierarchical layout system. A widget can either have a fixed size or be part of a flexible layout.

*   **`set_position(x, y)`**: Sets the local coordinates relative to the parent.
*   **`set_size(width, height)`**: Sets the explicit dimensions.
*   **`set_fixed_size(size)`**: Sets a fixed dimension for the widget in its parent's primary layout axis (height in vertical, width in horizontal).
*   **`set_margin(margin)`**: Internal space between the widget border and its content.
*   **`set_spacing(spacing)`**: Space between children in a layout.

### 2. Layout Types (`WidgetLayoutTypes`)
*   **`WIDGET_LAYOUT_HORIZONTAL`**: Arranges children side-by-side.
*   **`WIDGET_LAYOUT_VERTICAL`**: stacks children top-to-bottom.

### 3. Position Types (`WidgetPositionTypes`)
*   **`FILL`**: The widget expands to take up available space in the parent's layout. If multiple siblings are `FILL`, they share the space.
*   **`FREE`**: The widget is positioned at its absolute `x, y` coordinates, ignoring the parent's layout flow.

## Styling
Widgets support several visual properties that can be customized:

*   **`set_background_color(Color)`**: Sets the fill color.
*   **`set_border_radius(radius)`**: Rounds the corners.
*   **`set_border_width(width)`** and **`set_border_color(Color)`**: Configures the widget outline.
*   **`set_accent_color(WidgetAccentColor)`**: Uses theme-predefined colors (Primary, Success, Warning, Error, etc.).

## Event Handling
The `Widget` class uses an `EventsManager` system. You can connect lambdas or member functions to various user interactions:

```cpp
widget->when_click.connect([](MouseButtonEventContext &ev) {
    // Handle click
});

widget->when_mouse_enter.connect([](EventContext &ev) {
    // Handle hover start
});
```

**Common Events:**
*   `when_mouse_press` / `when_mouse_release` / `when_click`
*   `when_mouse_enter` / `when_mouse_leave`
*   `when_key_press` / `when_key_release`
*   `when_focus` / `when_blur`

## Lifecycle and Hierarchy
*   **`add_child(unique_ptr<Widget>)`**: Transfers ownership of a widget to become a child.
*   **`parent()`**: Returns the parent widget.
*   **`application()`**: Returns the `WaylandWindow` that owns this widget tree.
*   **`invalidate()`**: Requests a redraw of the widget and its parents.

## Building Layouts (The "Horizon Way")
To build complex, responsive layouts, avoid hardcoded sizes. Instead, nest vertical and horizontal widgets using `FILL` position types.

**Example: A Centered Header**
```cpp
auto header = std::make_unique<Widget>();
header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
header->set_fixed_size(60); // Fixed height

auto icon = std::make_unique<Icon>("app-icon");
icon->set_fixed_size(48); // Fixed width

auto title = std::make_unique<Label>("My App");
// Title is FILL by default, so it takes the remaining width

header->add_child(std::move(icon));
header->add_child(std::move(title));
```
