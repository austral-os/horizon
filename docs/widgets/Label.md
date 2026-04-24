# Label Widget

The `Label` widget is used to display text. It supports multi-line rendering, text alignment, and various font styles.

## Properties

*   **`set_text(string)`**: Sets the content of the label.
*   **`set_font_size(int)`**: Sets the text size. Use `-1` to use the theme default.
*   **`set_font_weight(FontWeight)`**: Supports `FONT_WEIGHT_NORMAL` and `FONT_WEIGHT_BOLD`.
*   **`set_text_color(Color)`**: Sets the foreground color.
*   **`set_alignment(TextAlignment)`**: Horizontal alignment (`Left`, `Center`, `Right`).
*   **`set_vertical_alignment(VerticalAlignment)`**: Vertical alignment within the widget's box (`Top`, `Middle`, `Bottom`).

## Automatic Wrapping
The `Label` class automatically calculates text wrapping based on its available width. It uses the `calculate_lines` internal method to break text into multiple lines during the rendering phase.

## Usage Example

```cpp
auto title = std::make_unique<Label>("Welcome to Horizon");
title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
title->set_font_size(24);
title->set_alignment(TextAlignment::Center);
title->set_fixed_size(40); // Sets fixed height for the label
```
