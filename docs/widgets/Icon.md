# Icon Widget

The `Icon` widget displays vector (SVG) or raster images from the system's icon theme. It uses the `IconThemeLookup` service to find the most appropriate file for the current theme and requested size.

## Properties

*   **`set_icon_name(string)`**: The name of the icon in the theme (e.g., "utilities-terminal", "folder").
*   **`set_icon_size(int)`**: The target resolution in pixels. The widget will try to find the closest match in the theme.
*   **`set_fixed_size(int)`**: Sets the widget container size. Usually matches the icon size plus some margin.

## Usage Example

```cpp
auto icon = std::make_unique<Icon>();
icon->set_icon_name("document-open");
icon->set_icon_size(24);
icon->set_fixed_size(32); // Container is slightly larger than the icon
```
