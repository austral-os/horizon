# Preferences Dialog System

The Preferences Dialog System provides a standardized way for applications to manage and display user settings. It integrates directly with the `WaylandWindow` base class and the system's global menu.

## Overview

Applications can declare their preferences by providing a **Factory Callback**. This factory is responsible for constructing a `PreferencesContent` widget whenever the user requests to see the settings.

### Key Benefits
- **Automated Menu Integration**: If a factory is set, a "Preferences" item is automatically added to the application's global menu (usually under the App Menu in the top panel).
- **Shortcut Support**: Standard shortcuts like `Ctrl+,` are automatically handled.
- **Thread Safety**: The dialog runs in its own thread, ensuring the main application remains responsive.
- **Standardized Lifecycle**: The framework handles the window creation, toolbar setup, and destruction.

## Implementation Guide

### 1. Define your Configuration Sections
Create classes that inherit from `ConfigSection` and `Widget` to define your settings UI.

```cpp
class MySettingsSection : public Widget, public ConfigSection {
public:
    MySettingsSection() {
        // Setup UI components (Sliders, Checkboxes, etc.)
    }

    void from_json(const nlohmann::json &j) override { /* Load state */ }
    nlohmann::json to_json() const override { /* Save state */ }
};
```

### 2. Register the Preferences Factory
In your main application window, use `set_preferences_content`. Instead of passing a single object, you pass a lambda that returns a new `unique_ptr<PreferencesContent>`.

```cpp
std::string config_path = "/home/user/.config/myapp.json";

app.set_preferences_content([config_path]() {
    auto content = std::make_unique<PreferencesContent>(config_path);
    
    // Add sections
    content->add_section("General", "preferences-system", std::make_unique<MySettingsSection>());
    
    return content;
});
```

### 3. Manual Invocation
While the global menu handles invocation automatically, you can also trigger the dialog manually from any part of your code:

```cpp
app.show_preferences();
```

## How it Works

1. **Declaration**: When `set_preferences_content` is called, `WaylandWindow` stores the lambda.
2. **Menu Initialization**: `init_global_menu()` checks for the factory and adds the "Preferences" item if present.
3. **Execution**: When clicked (or triggered via shortcut), a new thread is spawned. 
4. **Construction**: Inside that thread, the factory is called to get a fresh `PreferencesContent` instance.
5. **Display**: A `DialogPreferences` window is created, populated with the content, and shown to the user.

## Important Note on Factory Pattern
Widgets in Horizon cannot be shared between windows because each window runs in its own thread with its own graphics context. By using a factory pattern, we ensure that a fresh, valid widget is created every time the user opens the settings, even if they open it multiple times during a single session.
