# Horizon Localization (i18n) System

Horizon uses a robust, JSON-base internationalization system designed to be flexible during development and efficient in production.

## Key Features

- **JSON Backend**: All translations are stored in standard JSON files.
- **Hierarchical Keys**: Support for nested keys using dot notation (e.g., `app.menu.file`).
- **Dynamic Search Paths**: The framework automatically searches for locale files in multiple locations (current dir, project root, system paths).
- **Fallback Chain**: Automatic regional fallback (e.g., if `es_AR` is not found, it tries `es`, then `en`).
- **Deep Merge**: Applications can load multiple locale files (e.g., Core + App-specific) which are merged into a single translation object.
- **Pluralization**: Native support for plural forms.
- **Interpolation**: Variables using the `{var_name}` syntax within strings.

---

## File Structure

Locale files should be placed in a `locales` directory:

```text
my_app/
├── CMakeLists.txt
├── locales/
│   ├── en.json
│   └── es.json
└── src/
    └── main.cpp
```

### Example JSON (`es.json`)
```json
{
  "my_app": {
    "title": "Mi Aplicación",
    "greeting": "¡Hola, {name}!",
    "items": {
      "one": "Tienes un solo objeto",
      "other": "Tienes {count} objetos"
    }
  }
}
```

---

## Usage in Code

### 1. Initialization and Loading
In your `main()` or Application constructor:

```cpp
#include <horizon/I18n.hpp>

// ...
// 1. Load your app-specific locales
horizon::i18n().load_app_locales("my_app");
```

### 2. Simple Translation
```cpp
// Returns "Mi Aplicación" if in Spanish
std::string title = horizon::i18n().tr("my_app.title");
```

### 3. Using Variables (Interpolation)
```cpp
// Returns "¡Hola, Horacio!"
std::string msg = horizon::i18n().tr("my_app.greeting", {{"name", "Horacio"}});
```

### 4. Pluralization
```cpp
// Returns "Tienes 5 objetos"
std::string status = horizon::i18n().tr("my_app.items", 5);
```

---

## Search Paths Strategy

Horizon searches for `[locale].json` files in the following order:
1. `./locales/`
2. `./apps/[app_id]/locales/`
3. `./libs/[app_id]/locales/`
4. `/usr/share/horizon/...`

This means during development, simply having a `locales/` folder next to your binary is enough.

---

## CMake Integration

To ensure your locales are available to your binary in the `build/` directory, add this to your app's `CMakeLists.txt`:

```cmake
# Automatically copy the locales folder to the build directory post-build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
)
```

---

## Minimal Example Application

```cpp
#include <horizon/WaylandWindow.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>

int main(int argc, char** argv) {
    // Basic setup
    horizon::WaylandWindow app("org.horizon.example", 400, 300);

    // Load translations for this app ID
    horizon::i18n().load_app_locales("example_app");

    // Use translations
    app.set_name(horizon::i18n().tr("example_app.title"));

    auto label = std::make_unique<horizon::Label>(
        horizon::i18n().tr("example_app.welcome_message")
    );
    app.set_root(std::move(label));

    return app.run();
}
```
