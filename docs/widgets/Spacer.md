# Spacer Helper

The `Spacer` is not a class but a set of helper functions that return a plain `Widget` configured to act as a placeholder in a layout. It is essential for alignment and distributing empty space.

## Functions

*   **`Spacer()`**: Returns a widget with `FILL` position type. In a layout, this will expand to take up all remaining free space, pushing other widgets to the edges.
*   **`Spacer(int size)`**: Returns a widget with a fixed size. This acts as a static gap between other widgets.

## Usage Scenarios

### 1. Centering a Widget
Putting a `Spacer()` before and after a widget in a horizontal layout will center it.

```cpp
layout->add_child(Spacer());
layout->add_child(std::move(my_widget));
layout->add_child(Spacer());
```

### 2. Pushing Content to the Bottom
In a vertical layout, adding a `Spacer()` at the top will push all subsequent widgets to the bottom of the container.

```cpp
layout->add_child(Spacer());
layout->add_child(std::move(submit_button));
```
