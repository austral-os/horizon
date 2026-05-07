# Vault

The `Vault` widget is a floating popover container designed to display interactive UI components relative to an "owner" widget, such as a toolbar button. Unlike standard context menus, a `Vault` allows full interaction with complex child widgets (like text boxes, sliders, or scroll areas) without automatically closing when you click inside it.

## Key Concepts

* **Interactive Popover**: A Vault acts like an independent, floating panel. The user can type into TextBoxes or drag Sliders inside a Vault. The Vault will only close if the user explicitly clicks outside of it (on the main window).
* **Relative Positioning**: The Vault automatically detects where its owner widget is located on the screen and positions itself accordingly. It also dynamically draws a pointing arrow (a "beak") that points back to the owner widget to give the user visual context.
* **Auto-Sizing**: The Vault calculates its size automatically based on the preferred size of its content widget. 

## When to use a Vault vs a Menu

* Use a **Menu** when you want a simple list of actionable items (`MenuItem`) that close immediately after an option is selected.
* Use a **Vault** when you want to group settings, complex controls, or forms (e.g., a brightness slider, a search input, or a list of toggles) in a popover that remains open while the user interacts with it.

---

## Usage Example

To use a Vault, you generally create a base layout container (e.g., a `Widget` with a vertical layout), add your interactive child components to it, set this container as the Vault's content, and finally link the Vault to its owner (like a `ToolbarButton`).

> **Important Layout Tip**: When adding widgets to a vertical layout inside a Vault, it is highly recommended to explicitly set their fixed size (e.g., `set_fixed_size()`) for widgets that require it. If you do not set a fixed size, the layout engine will treat them as "free-space" widgets and may squash them evenly.

```cpp
#include <horizon/Vault.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/TextBox.hpp>

// ... inside your window or layout setup ...

// 1. Create the owner widget (the button that will open the Vault)
auto gear_btn = std::make_unique<ToolbarButton>("Settings", "settings-gear-symbolic");
gear_btn->set_fixed_size(60);

// 2. Create the Vault instance
auto vault = std::make_unique<Vault>();

// 3. Create the content container for the vault
auto vault_content = std::make_unique<Widget>();
vault_content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
vault_content->set_spacing(15);
vault_content->set_width(300); // Set a base width for the Vault

// 4. Populate the content container
auto title = std::make_unique<Label>("Vault Settings");
title->set_font_weight(FONT_WEIGHT_BOLD);
title->set_fixed_size(24); // Give it a fixed height so it doesn't get squashed
vault_content->add_child(std::move(title));

auto slider_label = std::make_unique<Label>("Brightness Control");
slider_label->set_fixed_size(20);
vault_content->add_child(std::move(slider_label));

auto slider = std::make_unique<Slider>();
// Slider has a default fixed height of 40px automatically.
vault_content->add_child(std::move(slider));

auto search = std::make_unique<TextBox<>>();
search->set_placeholder("Quick search...");
// TextBox has a default fixed height of 40px automatically.
vault_content->add_child(std::move(search));

// 5. Set the populated container as the content of the Vault
vault->set_content(std::move(vault_content));

// 6. Associate the Vault with the owner widget
// This automatically handles the opening of the vault when the button is clicked.
gear_btn->set_vault(std::move(vault));

// 7. Add the button to your layout
toolbar->add_child(std::move(gear_btn));
```

## How it works under the hood

1. When the user clicks the `gear_btn`, the button calls `application()->show_vault(m_vault.get(), -1, -1, serial, this);`.
2. The `WaylandWindow` system intercepts this call. It calculates the absolute position of the button on the screen.
3. Based on the position and available screen space, it decides whether to display the Vault to the right, left, top, or bottom of the button.
4. It tells the `Vault` to adjust its internal padding and draw an arrow pointing towards the center of the button.
5. The Vault calculates its size by reading the `preferred_height` and `preferred_width` of its internal `content()` widget, adjusting its Wayland surface dimensions.
6. The Wayland compositor maps the popup. Pointer events sent to the Vault are passed directly to the interactive child widgets. Click events outside the Vault are caught by the main window and used to close the Vault seamlessly.
