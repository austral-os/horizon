# ScrollArea Widget

The `ScrollArea` provides a viewport into a larger widget. It automatically manages vertical and horizontal scrollbars (using the `AquaPolygon` style) based on the size of the content.

## Key Methods

*   **`set_content(unique_ptr<Widget>)`**: Sets the widget to be scrolled. This widget is treated as the single child of the scroll area.
*   **`set_scroll_position(x, y)`**: Programmatically moves the scroll viewport.

## Responsive Width Handling
The `ScrollArea` automatically detects if its content is set to `FILL`. In this case, it will force the content's width to match the available space (viewport width minus scrollbar thickness), ensuring a perfectly responsive layout where only vertical scrolling is enabled.

## Usage Example

```cpp
auto scroll = std::make_unique<ScrollArea>();
auto content = std::make_unique<Widget>();
content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
content->set_position_type(FILL); // Enable responsive width
content->set_height(2000); // Very tall content

// Add many items to content...

scroll->set_content(std::move(content));
```
