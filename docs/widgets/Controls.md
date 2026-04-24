# Checkbox Widget

The `Checkbox` widget allows the user to toggle a binary state (on/off).

## Properties
*   **`set_checked(bool)`**: Sets the state of the checkbox.
*   **`is_checked()`**: Returns the current state.
*   **`set_text(string)`**: Sets the label next to the checkbox.

## Events
*   **`when_toggle`**: Dispatched whenever the state changes.

---

# RadioButton Widget

`RadioButton` widgets are used in groups where only one option can be selected at a time.

## Usage
Radio buttons should be added to a container that manages their mutual exclusivity.

---

# Slider Widget

The `Slider` allows users to select a value from a continuous or discrete range by moving a thumb along a track.

## Properties
*   **`set_range(min, max)`**: Sets the value bounds.
*   **`set_value(float)`**: Sets the current value.
*   **`value()`**: Gets the current value.

---

# ProgressBar Widget

`ProgressBar` provides visual feedback on the progress of an operation.

## Properties
*   **`set_progress(float)`**: Value between 0.0 and 1.0.
*   **`set_text(string)`**: Optional text to overlay on the bar.

---

# TextBox and Textarea Widgets

These widgets allow for single-line and multi-line text input respectively.

## TextBox
*   **`set_text(string)`**: Current content.
*   **`set_placeholder(string)`**: Hint text when empty.
*   **`set_is_password(bool)`**: Masks input for sensitive data.

## Textarea
A more complex widget for large text blocks, supporting scrolling and basic formatting.

---

# Combo Widget

The `Combo` (Combobox) allows selecting an item from a dropdown list.

## Methods
*   **`add_item(string)`**: Adds an option to the list.
*   **`set_current_index(int)`**: Selects an item.
*   **`current_text()`**: Returns the selected string.
