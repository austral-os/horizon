# Horizon Localization (i18n) System

Horizon uses a JSON-based internationalization system with two independent loading
modes: **global-based** (the app follows the system locale chain) and **app-specific
locale override** (the app picks a specific locale independent of the system).

## Quick Start

The simplest path — your app follows the system locale automatically:

```cpp
#include <horizon/I18n.hpp>

// In main(): load translations for your app ID
horizon::i18n().load_app_locales("my_app");

// Use translations anywhere
label->set_text(horizon::i18n().tr("my_app.welcome"));
```

Place locale JSON files in `locales/` next to your binary or in the standard
search paths:

```text
apps/my_app/locales/
├── en.json
├── es.json
└── pt.json
```

That's it for the happy path. The rest of this document covers the full system.

---

## Locale File Structure

Each locale file is a JSON document with **language metadata** at the top level
and translation keys nested under sections:

```json
{
    "language": {
        "local_name": "Português",
        "name": "Portuguese"
    },
    "my_app": {
        "title": "Meu Aplicativo",
        "greeting": "Olá, {name}!",
        "items": {
            "one": "Você tem um item",
            "other": "Você tem {count} itens"
        }
    }
}
```

| Field | Required | Purpose |
|-------|----------|---------|
| `language.local_name` | Recommended | Display name in the language's own script. Shown in preferences combos. |
| `language.name` | Optional (fallback) | Display name in English. Used if `local_name` is absent. |
| `app_id.section.key` | Yes | Nested translation keys accessed via dot notation. |

If the file lacks `language.local_name` entirely, `get_app_locale_display_name()`
falls back to `language.name`, then to the locale code itself (e.g. `"fr"`).

---

## Two Loading Modes

### Mode 1: Global-Based (default)

Loads the locale chain derived from the global system locale. This is what most
apps want — follow the user's system language:

```cpp
// Resolves es_AR -> [es_AR, es, en] if current locale is es_AR
// Loads every file it can find in that chain
i18n().load_app_locales("my_app");
```

The chain is built by `resolve_locale_chain()`: `"es_AR"` → `["es_AR", "es"]`,
then `"en"` is appended as final fallback.

### Mode 2: App-Specific Locale Override

Loads a **single specific locale** regardless of what the global system locale
says. Used when the user has chosen an app-specific language preference:

```cpp
// Load ONLY Portuguese files, even if the system is in Spanish
bool ok = i18n().load_app_locale("my_app", "pt");
if (!ok) {
    // The requested locale has no files — fall back to global
    i18n().load_app_locales("my_app");
}
```

`load_app_locale()` returns `false` if no translation files exist for the
requested locale chain (e.g., `"fr"` → `["fr", "fr"]` → no files found).
The global chain and English are still loaded as fallback translations,
but the **current locale is NOT switched** so the app can safely fall back
to the global system locale instead of displaying a broken override.

> **Key distinction**: `load_app_locales()` uses the **system locale** as the
> starting point for its chain. `load_app_locale()` uses the **requested locale**
> as the starting point, keeps the global locale intact for fallback, and never
> redefines the system locale. The two calls are independent — you can call
> both in the same session without interference.

---

## API Reference

### `i18n().load_app_locales(app_id)`

Loads translations for `app_id` using the current system locale as the starting
point of the fallback chain.

```cpp
// System locale is "pt_BR"
i18n().load_app_locales("my_app");
// Chain: pt_BR -> pt -> en
// Tries each path for each locale code in order
```

| Returns | Meaning |
|---------|---------|
| `true` | At least one translation file was loaded |
| `false` | No files found anywhere in the chain |

---

### `i18n().load_app_locale(app_id, locale)`

Loads translations for `app_id` but starts from a **different locale** than the
system. The global locale remains available as a secondary fallback.

```cpp
// System locale is "es", but we want French
if (i18n().load_app_locale("my_app", "fr")) {
    // Current locale is now "fr"
    // Fallback chain: fr -> fr -> es -> en
    //                (requested) (global) (final)
} else {
    // No French files found. Current locale unchanged.
    // Safe to call load_app_locales() as fallback.
    i18n().load_app_locales("my_app");
}
```

**Fallback chain construction** (in order):

1. Requested locale + its regional chain (e.g. `"fr_CH"` → `["fr_CH", "fr"]`)
2. Global/system locale + its chain (if different from requested)
3. `"en"` always appended as final fallback

| Returns | Meaning |
|---------|---------|
| `true` | Requested locale chain found files — current locale switched |
| `false` | Requested locale not found at all — current locale **unchanged** |

> **Important:** `load_app_locale()` returns `false` when the requested locale
> has no files, even if the global fallback or English files loaded
> successfully. This lets callers distinguish "the user chose French but there
> are no French files" from "everything loaded fine."

---

### `i18n().available_app_locales(app_id)`

Scans the filesystem for locale files in app-specific paths and returns their
codes sorted alphabetically:

```cpp
auto locales = i18n().available_app_locales("text-editor");
// Returns: ["de", "en", "es", "fr", "it", "pt"]
```

This scans **only** app-specific directories (`apps/{app_id}/locales/`,
`libs/{app_id}/locales/`, `{app_id}/locales/`) — it does NOT scan the shared
`locales/` directory, avoiding false positives from core locale files.

---

### `i18n().get_app_locale_display_name(app_id, locale)`

Returns the human-readable name of a locale for display in UI combos:

```cpp
std::string name = i18n().get_app_locale_display_name("text-editor", "pt");
// Returns "Português" (from pt.json: language.local_name)
```

Resolution order per locale file:

1. `language.local_name` (name in the language's own script)
2. `language.name` (name in English)
3. The locale code itself (fallback)

---

### `i18n().set_locale(locale)`

Sets the **global/system locale**. Affects BOTH the current active locale AND
the global fallback used by the backend. This is what the framework calls
during `i18n()` initialization from the `LANG` environment variable.

```cpp
// From the global i18n() initializer:
instance.set_locale("es_AR");
// m_global_locale = "es_AR"
// m_current_locale = "es_AR"
// Backend global fallback = "es_AR"
```

---

### `i18n().set_current_locale(locale)`

Switches only the active locale WITHOUT touching the global fallback. Used
internally by `load_app_locale()` to apply an override without redefining the
system-wide fallback chain. Not typically called directly in app code.

---

### `i18n().current_locale()`

Returns the currently active locale code:

```cpp
std::string active = i18n().current_locale();
// Returns "fr" if load_app_locale("my_app", "fr") was called
```

---

### `i18n().tr(key, ...)`

Translate a key with optional interpolation and pluralization:

```cpp
// Simple
label->set_text(i18n().tr("my_app.title"));

// With variables
std::string msg = i18n().tr("my_app.greeting", {{"name", "Horacio"}});

// With pluralization
std::string status = i18n().tr("my_app.items", 5);
```

---

### `i18n().resolve_locale_chain(locale)`

Static utility that expands a locale code into its fallback chain:

```cpp
auto chain = I18n::resolve_locale_chain("es_AR");
// Returns: ["es_AR", "es"]

auto chain = I18n::resolve_locale_chain("fr");
// Returns: ["fr"]
```

---

## Search Paths

Horizon searches for locale files in this order:

1. `./locales/`
2. `./apps/{app_id}/locales/`
3. `./libs/{app_id}/locales/`
4. `./{app_id}/locales/`
5. `/usr/share/horizon/...`

The search path is configurable:

```cpp
I18n::add_search_path("/custom/path");
I18n::set_search_paths({".", "/usr/share/horizon"});
```

Default search paths are set in the `i18n()` initializer:
- `.` (current working directory)
- `HORIZON_SOURCE_DIR` and `HORIZON_SOURCE_DIR/share` (when compiled with the define)
- `/usr/share/horizon`

---

## CMake Integration

```cmake
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
)
```

For apps using `horizon_install_app`, locales are handled automatically during
installation.

---

## Complete Startup Patterns

### Pattern A: Global-Based (system locale follows)

Suitable for apps that don't offer a language preference. One call, done:

```cpp
int main(int argc, char** argv) {
    Application app("org.horizon.myapp", 800, 600);

    // Load translations following the system locale
    i18n().load_app_locales("my_app");

    // Build UI with translations
    app.set_name(i18n().tr("my_app.title"));
    auto window = std::make_unique<MyWindow>();
    app.set_root(std::move(window));
    app.run();
}
```

### Pattern B: Persisted Language Override (text-editor style)

For apps that persist a language choice and load it before building the UI:

```cpp
int main(int argc, char** argv) {
    Application app("org.horizon.text-editor", 1024, 768);

    // 1. First pass: load with system locale for the initial i18n calls
    i18n().load_app_locales("text-editor");

    // 2. Set up preferences with the app title (translated)
    app.set_name(i18n().tr("text_editor.title"));

    // 3. Set up preferences factory (not yet showing, just registering)
    app.set_preferences_content([config_path]() {
        auto content = std::make_unique<PreferencesContent>(config_path);
        // TextEditorGeneralSection handles language Combo internally
        content->add_section("General", "preferences-system",
            std::make_unique<text_editor::TextEditorGeneralSection>(...));
        return content;
    });

    // 4. Create window — its constructor reads persisted language
    //    and reloads if needed BEFORE building the UI
    auto window = std::make_unique<text_editor::TextEditorWindow>();
    // Inside TextEditorWindow():
    //   a. load_language_setting() -> reads config, returns "fr" or "default"
    //   b. If a specific locale, calls load_app_locale("text-editor", "fr")
    //   c. If load_app_locale fails, falls back to load_app_locales()
    //   d. Then calls setup_ui() — all i18n.tr() calls use the resolved locale

    app.set_root(std::move(window));
    app.run();
}
```

The critical detail in `TextEditorWindow`'s constructor:

```cpp
TextEditorWindow::TextEditorWindow()
    : ApplicationWindow("Text Editor")
{
    // Load language preference from config BEFORE setting title or building UI
    std::string lang = load_language_setting();
    if (lang.empty() || lang == "default") {
        i18n().load_app_locales("text-editor");
    } else {
        if (!i18n().load_app_locale("text-editor", lang)) {
            // Invalid or missing locale — fall back to global default
            LOG_WARNING << "language '" << lang << "' not found, falling back";
            i18n().load_app_locales("text-editor");
        }
    }

    // Now all i18n calls use the correct locale
    set_title(i18n().tr("text_editor.title"));
    setup_ui();
}
```

The `load_language_setting()` helper reads the persisted value from the
app's config JSON:

```cpp
std::string TextEditorWindow::load_language_setting() {
    // Read from ~/.config/horizon/text-editor.json
    nlohmann::json j;
    file >> j;
    if (j.contains("editor") && j["editor"].contains("language")) {
        return j["editor"]["language"].get<std::string>();
    }
    return "default";
}
```

**Why this order matters**: If you set the title or build widgets before
loading the app-specific locale, the user interface briefly uses the wrong
language. The persisted locale must be applied **between** the initial
`load_app_locales()` (which may use the system locale for early setup) and
the UI construction.

---

## Preferences UI for Language Selection

When your app offers a language preference in its settings, use a `Combo`
populated with `available_app_locales()` and `get_app_locale_display_name()`:

```cpp
auto lang_combo = std::make_unique<Combo>();
lang_combo->set_width(250);

// "Default (System)" — always first item
lang_combo->add_item("default", i18n().tr("my_app.preferences.language_default"));

// Scan available locale files and add them sorted
auto available = i18n().available_app_locales("my_app");
for (const auto& loc : available) {
    std::string display = i18n().get_app_locale_display_name("my_app", loc);
    lang_combo->add_item(loc, display);
}

// On selection, show a restart-required alert
// Live retranslation is NOT required
lang_combo->when_item_selected.connect([this](const ComboItemSelectedContext &ctx) {
    if (m_loading) return;  // skip during from_json()
    if (m_on_change) m_on_change();

    // Language changes only take effect after restart
    if (application()) {
        application()->alert(
            i18n().tr("my_app.preferences.language_restart"),
            i18n().tr("my_app.title"),
            MessageType::Info);
    }
});
```

**Required locale keys**:

| Key | Example (en.json) | Purpose |
|-----|-------------------|---------|
| `app_id.preferences.language` | `"Language"` | Label above the language combo |
| `app_id.preferences.language_default` | `"Default (System)"` | First combo item — means "use global locale" |
| `app_id.preferences.language_restart` | `"Language changes will take effect after restart."` | Alert body on selection change |

**Persistence** (via `ConfigSection`):

```cpp
// from_json() — restore selection
m_loading = true;
m_language_combo->set_selected_item_by_id(j.value("language", "default"));
m_loading = false;

// to_json() — persist selection
if (auto selected = m_language_combo->selected_item()) {
    j["language"] = selected->id;
} else {
    j["language"] = "default";
}
```

---

## Fallback Contract

When a translation key is not found, the system resolves it through this chain:

```
For load_app_locales():
  app's (current locale) file
    → app's (regional fallback) file     e.g. "es_AR" → "es"
      → app's "en" file                    (final fallback)
        → key itself returned as text      (last resort)

For load_app_locale("app_id", "fr"):
  app's "fr" file                           (requested)
    → app's "fr" regional fallback          (e.g., fr_CH → fr)
      → app's global/system locale file     (secondary fallback)
        → app's "en" file                   (final fallback)
          → key itself returned as text
```

The global locale serves as a **secondary fallback** when the requested
app-specific override is active. If a key exists in the global-locale file
but not in the requested locale, the global version is returned before
falling back to English.

---

## Handling Invalid Persisted Locales

If the user's config file contains a language code that no longer exists
(e.g., the app dropped support for a locale), the startup flow must handle
it gracefully:

```cpp
std::string lang = load_language_setting();
if (lang != "default") {
    if (!i18n().load_app_locale("text-editor", lang)) {
        // Locale not available — log and fall back
        LOG_WARNING << "Persisted language '" << lang << "' not found";
        i18n().load_app_locales("text-editor");

        // Optionally: reset the config to "default"
        // so the user sees the correct state next time
    }
}
```

`load_app_locale()` returns `false` when the requested locale has no files,
so the calling code can detect this and fall back without displaying a broken
interface.

---

## Minimal Example Application

```cpp
#include <horizon/WaylandWindow.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>

int main(int argc, char** argv) {
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

---

## Checklist / Gotchas

- [ ] Every locale JSON file includes `language.local_name` for the combo UI.
- [ ] `load_app_locale()` return value is checked — if `false`, fall back to
      `load_app_locales()`.
- [ ] Persisted language is loaded **before** UI construction, not after.
- [ ] Combo includes `"default"` as the first item for "use system locale."
- [ ] Combo `when_item_selected` checks `m_loading` guard to prevent
      side effects during `from_json()`.
- [ ] Restart-required alert shown on language combo change — live
      retranslation is not required.
- [ ] `available_app_locales()` only scans app-specific locale directories;
      core locales are excluded from the result.
- [ ] `get_app_locale_display_name()` reads from `language.local_name` first,
      then `language.name`, then falls back to the code.
- [ ] Global-based mode (`load_app_locales`) and override mode
      (`load_app_locale`) can be mixed safely — they don't interfere.
- [ ] The text-editor stores language under the `editor` config section as
      `"language"`, not at the top level.
- [ ] If the persisted language value is invalid or missing, treat it as
      `"default"` — never crash or show an empty combo.
