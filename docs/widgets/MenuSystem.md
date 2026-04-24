# Menu System

Horizon features a hierarchical menu system used for context menus, application menus, and the global system bar.

## Components

### 1. Menu
The main container for menu items. It can be shown as a popup or attached to a `MenuBar`.

### 2. MenuBar
A horizontal bar typically placed at the top of a window or the screen to hold several top-level `MenuItem`s.

### 3. MenuItem
A single entry in a menu.
*   Supports labels and icons.
*   Can hold a sub-menu for hierarchical navigation.
*   Dispatches `when_click` when selected.

### 4. MenuSeparator
A visual line to group related items within a `Menu`.

## Global Menu Integration
Applications using `horizon::Application` automatically participate in the global menu system. You can define your application's menu structure, and the framework will handle its presentation in the top panel.

```cpp
auto file_menu = std::make_unique<Menu>();
file_menu->add_item("Open", "document-open", []() { /* handle */ });
file_menu->add_separator();
file_menu->add_item("Quit", "application-exit", []() { /* handle */ });

app.set_main_menu(std::move(file_menu));
```
