# Window Widget

The `Window` widget (not to be confused with `WaylandWindow`) is a decorative and structural container that provides a standard Horizon window frame, including a titlebar and borders.

## Components

A `Window` typically consists of:
1.  **Titlebar**: Contains the title text and window control buttons (Close, Maximize, Minimize).
2.  **Content Area**: The main space where the application's UI is placed via `add_child`.
3.  **Frame**: The surrounding border with glassmorphism effects.

## Usage

In Horizon applications, you usually set a `Window` as the root of your `Application`.

```cpp
auto win_frame = std::make_unique<Window>("My Application");
win_frame->set_size(800, 600);

auto my_ui = std::make_unique<Widget>();
// ... setup UI ...

win_frame->add_child(std::move(my_ui));
app.set_root(std::move(win_frame));
```

## Key Properties
*   **`set_title(string)`**: Updates the text shown in the titlebar.
*   **`set_size(w, h)`**: Sets the outer dimensions of the window. The content area will be automatically calculated based on the titlebar height and margins.
