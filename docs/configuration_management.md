# Configuration Management System

The Horizon framework provides a robust and standardized way to manage application preferences and configuration files using the JSON format. This system is centered around two core classes: `ConfigManager` and `ConfigSection`.

## 📋 Overview

The configuration system is designed to be modular:
- **`ConfigManager`**: Handles the physical file on disk (loading, saving, and path resolution).
- **`ConfigSection`**: An interface implemented by any component that needs to persist its state.

### How they work together

A typical application will have one `ConfigManager` per configuration file (e.g., `desktop.json`). This file can contain multiple sections, where each section corresponds to a specific component or feature that implements the `ConfigSection` interface.

---

## 🛠️ Implementing `ConfigSection`

To make a class "configurable", it must inherit from `horizon::ConfigSection` and implement two virtual methods: `from_json` and `to_json`.

### 1. Header Definition

```cpp
#include <horizon/ConfigSection.hpp>
#include <nlohmann/json.hpp>

class MyComponent : public horizon::Widget, public horizon::ConfigSection {
public:
    // ...
    
    // ConfigSection implementation
    void from_json(const nlohmann::json& j) override;
    nlohmann::json to_json() const override;

private:
    int m_preference_value = 10;
};
```

### 2. Implementation

```cpp
void MyComponent::from_json(const nlohmann::json& j) {
    if (j.is_null()) return;
    
    // Use .value() to provide safe defaults
    m_preference_value = j.value("preference_value", 10);
}

nlohmann::json MyComponent::to_json() const {
    nlohmann::json j;
    j["preference_value"] = m_preference_value;
    return j;
}
```

---

## 🚀 Using `ConfigManager`

The `ConfigManager` is responsible for reading and writing the JSON data to a file.

### 1. Initialization

Usually, you'll want to store configuration files in the standard site-wide or user-specific configuration directory (`~/.config/horizon/`).

```cpp
#include <horizon/ConfigManager.hpp>

// Create a manager for a specific file
auto config = std::make_unique<horizon::ConfigManager>("/home/user/.config/horizon/my_app.json");

// Load the data from disk
if (config->load()) {
    // Data loaded successfully
}
```

### 2. Connecting with a Section

Once the manager is loaded, you can pass individual sections to your components.

```cpp
// Load a specific section into a component
my_component->from_json(config->get_section("settings"));
```

### 3. Saving Changes

When a value changes and you want to persist it:

```cpp
// Update the section in the manager
config->set_section("settings", my_component->to_json());

// Save the entire file back to disk
config->save();
```

---

## 💡 Best Practices

### Path Resolution
Don't hardcode paths. Use the framework's utilities (if available in your application context) to resolve standard configuration directories.

### Safe Loading
Always check `j.is_null()` in `from_json` to handle cases where the configuration file or section doesn't exist yet (e.g., on the first run).

### Nested Data
`ConfigManager` also provides convenience methods for deep-nested values:
- `get_value(section, key, default)`
- `set_value(section, key, value)`

These allow you to modify specific values without manually serializing the entire section every time.

---

## 📝 Complete Example

```cpp
#include <horizon/ConfigManager.hpp>
#include <horizon/ConfigSection.hpp>

class SettingsView : public horizon::ConfigSection {
    std::string theme = "light";
    
    void from_json(const nlohmann::json& j) override {
        if (!j.is_null()) theme = j.value("theme", "light");
    }
    
    nlohmann::json to_json() const override {
        return { {"theme", theme} };
    }
};

// In your application logic:
void init_app() {
    auto config = std::make_unique<horizon::ConfigManager>(path);
    config->load();
    
    auto view = std::make_unique<SettingsView>();
    view->from_json(config->get_section("ui"));
    
    // ... after user changes theme ...
    config->set_section("ui", view->to_json());
    config->save();
}
```
