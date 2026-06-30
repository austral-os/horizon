# App-Specific Color Schemes

Horizon applications can provide their own color schemes without changing the global system theme. This lets an app offer variants such as `tokyo-night`, `dracula`, or `nord` while the rest of the desktop continues to use the default `light` / `dark` scheme from `~/.config/horizon/color-scheme.json`.

## Quick path

1. Create an `assets` directory inside your application.
2. Add an `assets/color-scheme.json` file using the same structure as the global Horizon color scheme.
3. Load and activate the app scheme from the application startup code.
4. Use `default` when the app should fall back to the global system scheme.

```text
apps/my-app/
├── CMakeLists.txt
├── assets/
│   └── color-scheme.json
├── locales/
└── src/
    └── main.cpp
```

## Color scheme file

The app file must be named:

```text
assets/color-scheme.json
```

It uses the same structure as the global system file. The complete shape is:

```json
{
  "colors": {
    "variant-id": {
      "air_border": "#000000",
      "air_default1": "#000000",
      "air_default2": "#000000",
      "aqua_default1": "#000000",
      "aqua_default2": "#000000",
      "aqua_highlight1": "#000000",
      "aqua_primary1": "#000000",
      "aqua_primary2": "#000000",
      "default1": "#000000",
      "default2": "#000000",
      "dock_glare": "#000000",
      "dock_lip1": "#000000",
      "dock_lip2": "#000000",
      "dock_shadow": "#000000",
      "dock_surface1": "#000000",
      "dock_surface2": "#000000",
      "error1": "#000000",
      "error2": "#000000",
      "group_alt_1": "#000000",
      "group_alt_2": "#000000",
      "group_separator": "#000000",
      "icon_fg": "#000000",
      "info1": "#000000",
      "info2": "#000000",
      "menu_bg": "#000000",
      "menu_bg1": "#000000",
      "menu_bg2": "#000000",
      "menu_border": "#000000",
      "menu_item_fg": "#000000",
      "menu_item_selected_bg1": "#000000",
      "menu_item_selected_bg2": "#000000",
      "menu_item_selected_fg": "#000000",
      "menu_item_shortcut_fg": "#000000",
      "menu_separator": "#000000",
      "notification_bg": "#000000",
      "notification_fg": "#000000",
      "panel_bg1": "#000000",
      "panel_bg2": "#000000",
      "panel_border": "#000000",
      "primary1": "#000000",
      "primary2": "#000000",
      "ribbon_tab_active_title_bg": "#000000",
      "ribbon_tab_title_bg": "#000000",
      "secondary1": "#000000",
      "secondary2": "#000000",
      "sidebar_bg": "#000000",
      "sidebar_border": "#000000",
      "sidebar_item_fg": "#000000",
      "sidebar_item_fg2": "#000000",
      "solid_border": "#000000",
      "solid_default": "#000000",
      "solid_highlight": "#000000",
      "solid_highlight2": "#000000",
      "solid_primary": "#000000",
      "success1": "#000000",
      "success2": "#000000",
      "tab_button_1": "#000000",
      "tab_button_2": "#000000",
      "tab_header_1": "#000000",
      "tab_header_2": "#000000",
      "table_row": "#000000",
      "table_row_alternate": "#000000",
      "table_row_fg": "#000000",
      "table_row_selected": "#000000",
      "table_row_selected_fg": "#000000",
      "textbox_bg": "#000000",
      "textbox_brd": "#000000",
      "textbox_fg": "#000000",
      "textbox_focus": "#000000",
      "textbox_invalid": "#000000",
      "textbox_ph_fg": "#000000",
      "titlebar_bg1": "#000000",
      "titlebar_bg2": "#000000",
      "titlebar_border": "#000000",
      "titlebar_fg": "#000000",
      "warning1": "#000000",
      "warning2": "#000000",
      "window_bg": "#000000",
      "window_bg_alt": "#000000",
      "window_border": "#000000",
      "window_fg": "#000000"
    }
  },
  "fonts": {
    "titlebar": {
      "family": "Lucida Grande",
      "size": 16,
      "weight": "bold"
    },
    "window": {
      "family": "Lucida Grande",
      "size": 20,
      "weight": "bold"
    }
  },
  "menu_opacity": 0.803398072719574,
  "panel_opacity": 0.6340579390525818,
  "variant": "variant-id"
}
```

The top-level `variant` value is the app's default variant. Variant names are app-scoped identifiers, not global enums, so they do not need to be `light` or `dark`.

For app-specific themes, `colors` may contain variants such as `tokyo-night`, `monokai`, or `dracula`. The role names inside each variant should match the Horizon color roles shown above. If a role is missing from the app variant, Horizon falls back to the global/default scheme for that role.

## Default light and dark configuration

The global Horizon scheme uses the same file shape with `light` and `dark` variants. The following is the default configuration currently used as the baseline for application schemes:

```json
{
  "colors": {
    "dark": {
      "air_border": "#1c1c1e",
      "air_default1": "#2c2c2e",
      "air_default2": "#3a3a3c",
      "aqua_default1": "#1c1c1e",
      "aqua_default2": "#2c2c2e",
      "aqua_highlight1": "#3a3a3c",
      "aqua_primary1": "#007aff",
      "aqua_primary2": "#0a84ff",
      "default1": "#1c1c1e",
      "default2": "#2c2c2e",
      "dock_glare": "#4a4a4c",
      "dock_lip1": "#3a3a3c",
      "dock_lip2": "#1c1c1e",
      "dock_shadow": "#000000",
      "dock_surface1": "#2c2c2e",
      "dock_surface2": "#1c1c1e",
      "error1": "#ff453a",
      "error2": "#ff6961",
      "group_alt_1": "#2c2c2e",
      "group_alt_2": "#3a3a3c",
      "group_separator": "#38383a",
      "icon_fg": "#98989d",
      "info1": "#0a84ff",
      "info2": "#64d2ff",
      "menu_bg": "#2c2c2e",
      "menu_bg1": "#1c1c1e",
      "menu_bg2": "#2c2c2e",
      "menu_border": "#3a3a3c",
      "menu_item_fg": "#ffffff",
      "menu_item_selected_bg1": "#007aff",
      "menu_item_selected_bg2": "#0056b3",
      "menu_item_selected_fg": "#ffffff",
      "menu_item_shortcut_fg": "#8e8e93",
      "menu_separator": "#38383a",
      "notification_bg": "#2c2c2e",
      "notification_fg": "#ffffff",
      "panel_bg1": "#1c1c1e",
      "panel_bg2": "#2c2c2e",
      "panel_border": "#3a3a3c",
      "primary1": "#007aff",
      "primary2": "#0a84ff",
      "ribbon_tab_active_title_bg": "#26262c",
      "ribbon_tab_title_bg": "#36363c",
      "secondary1": "#0a84ff",
      "secondary2": "#007aff",
      "sidebar_bg": "#36363c",
      "sidebar_border": "#3a3a3c",
      "sidebar_item_fg": "#98989d",
      "sidebar_item_fg2": "#ffffff",
      "solid_border": "#262626",
      "solid_default": "#3a3a3c",
      "solid_highlight": "#35353a",
      "solid_highlight2": "#636366",
      "solid_primary": "#007aff",
      "success1": "#34c759",
      "success2": "#30d158",
      "tab_button_1": "#1c1cae",
      "tab_button_2": "#2c2c2e",
      "tab_header_1": "#1c1c1e",
      "tab_header_2": "#2c2c2e",
      "table_row": "#1c1c1e",
      "table_row_alternate": "#2c2c2e",
      "table_row_fg": "#ffffff",
      "table_row_selected": "#007aff",
      "table_row_selected_fg": "#ffffff",
      "textbox_bg": "#1c1c1e",
      "textbox_brd": "#3a3a3c",
      "textbox_fg": "#ffffff",
      "textbox_focus": "#007aff",
      "textbox_invalid": "#ff453a",
      "textbox_ph_fg": "#8e8e93",
      "titlebar_bg1": "#1c1c1e",
      "titlebar_bg2": "#2c2c2e",
      "titlebar_border": "#3a3a3c",
      "titlebar_fg": "#ffffff",
      "warning1": "#ff9f0a",
      "warning2": "#ffb340",
      "window_bg": "#1c1c1e",
      "window_bg_alt": "#1f1f1f",
      "window_border": "#3a3a3c",
      "window_fg": "#dfdfdf"
    },
    "light": {
      "air_border": "#666666",
      "air_default1": "#ffffff",
      "air_default2": "#f5f5f5",
      "aqua_default1": "#616161",
      "aqua_default2": "#dddddd",
      "aqua_highlight1": "#f3f3f3",
      "aqua_primary1": "#1e4ce1",
      "aqua_primary2": "#61aff5",
      "default1": "#616161",
      "default2": "#dddddd",
      "dock_glare": "#ffffff",
      "dock_lip1": "#cccccc",
      "dock_lip2": "#666666",
      "dock_shadow": "#191919",
      "dock_surface1": "#ffffff",
      "dock_surface2": "#ffffff",
      "error1": "#e11e41",
      "error2": "#f5617f",
      "group_alt_1": "#f5f5f5",
      "group_alt_2": "#ffffff",
      "group_separator": "#cccccc",
      "icon_fg": "#616161",
      "info1": "#e1d31e",
      "info2": "#f5de61",
      "menu_bg": "#ffffff",
      "menu_bg1": "#ffffff",
      "menu_bg2": "#f1f1f1",
      "menu_border": "#b2b2b2",
      "menu_item_fg": "#000000",
      "menu_item_selected_bg1": "#3373e6",
      "menu_item_selected_bg2": "#1a59d9",
      "menu_item_selected_fg": "#ffffff",
      "menu_item_shortcut_fg": "#666666",
      "menu_separator": "#d9d9d9",
      "notification_bg": "#ffffff",
      "notification_fg": "#000000",
      "panel_bg1": "#e6e6e6",
      "panel_bg2": "#cccccc",
      "panel_border": "#b3b3b3",
      "primary1": "#1e4ce1",
      "primary2": "#61aff5",
      "ribbon_tab_active_title_bg": "#bdc6db",
      "ribbon_tab_title_bg": "#adb7cc",
      "secondary1": "#61aff5",
      "secondary2": "#1e4ce1",
      "sidebar_bg": "#dfe6ee",
      "sidebar_border": "#787878",
      "sidebar_item_fg": "#333333",
      "sidebar_item_fg2": "#ffffff",
      "solid_border": "#f3f3f3",
      "solid_default": "#ffffff",
      "solid_highlight": "#f3f3f3",
      "solid_highlight2": "#ffffff",
      "solid_primary": "#1e4ce1",
      "success1": "#1ee176",
      "success2": "#61f5a3",
      "tab_button_1": "#c2f2ff",
      "tab_button_2": "#b1b1c1",
      "tab_header_1": "#f2f2f2",
      "tab_header_2": "#d1d1d1",
      "table_row": "#ffffff",
      "table_row_alternate": "#f3f8ff",
      "table_row_fg": "#212121",
      "table_row_selected": "#1e4ce1",
      "table_row_selected_fg": "#ffffff",
      "textbox_bg": "#ffffff",
      "textbox_brd": "#666666",
      "textbox_fg": "#000000",
      "textbox_focus": "#66b3ff",
      "textbox_invalid": "#e11e41",
      "textbox_ph_fg": "#999999",
      "titlebar_bg1": "#d8d8d8",
      "titlebar_bg2": "#fafafa",
      "titlebar_border": "#787878",
      "titlebar_fg": "#212121",
      "warning1": "#e16b1e",
      "warning2": "#f59f61",
      "window_bg": "#f2f2f2",
      "window_bg_alt": "#f4f4f4",
      "window_border": "#787878",
      "window_fg": "#212121"
    }
  },
  "fonts": {
    "titlebar": {
      "family": "Lucida Grande",
      "size": 16,
      "weight": "bold"
    },
    "window": {
      "family": "Lucida Grande",
      "size": 20,
      "weight": "bold"
    }
  },
  "menu_opacity": 0.803398072719574,
  "panel_opacity": 0.6340579390525818,
  "variant": "light"
}
```

## Loading the app scheme

In the application startup code, load and activate the app color scheme:

```cpp
#include <horizon/ThemeManager.hpp>

// ...

auto &theme_manager = horizon::ThemeManager::instance();
theme_manager.load_app_color_scheme("my-app");
theme_manager.activate_app_color_scheme("my-app");
```

After activation, existing color lookups keep working normally:

```cpp
auto color = horizon::ThemeManager::instance().get_color("window_bg");
```

Application code does not need to pass the app id to every color lookup.

## Listing and selecting variants

Use the ThemeManager app APIs to build a settings selector:

```cpp
auto variants = theme_manager.app_color_scheme_variants("my-app");
```

The returned list always includes:

```text
default
```

`default` is a special option. It means the app should use the global Horizon scheme, including the current `light` / `dark` system variant.

Example selector values:

```text
default
tokyo-night
monokai
solar-light
dracula
nord
onedark
gruvbox
```

To select an app-specific variant:

```cpp
theme_manager.set_app_color_scheme_variant("my-app", "dracula");
```

To return to the system default scheme:

```cpp
theme_manager.set_app_color_scheme_variant("my-app", "default");
```

## Resolution behavior

When an app-specific scheme is active, `ThemeManager::get_color(role)` resolves colors in this order:

1. The active app variant, if it is not `default`.
2. The global user scheme from `~/.config/horizon/color-scheme.json`.
3. The built-in fallback scheme.

This means app schemes can provide only the roles they need. Missing roles fall back to the global/default theme instead of breaking the app.

## Build and install behavior

Apps using `horizon_install_app` get asset handling automatically.

During development, Ninja copies:

```text
apps/my-app/assets/
```

next to the built app executable, so the app can load the scheme from the build output.

During installation or `.deb` packaging, the asset is installed under:

```text
/usr/share/horizon/apps/my-app/color-scheme.json
```

This matches the existing per-app resource layout used by locales.

## Checklist

- [ ] The app has `assets/color-scheme.json`.
- [ ] The file has top-level `colors` and `variant` properties.
- [ ] Each variant uses strict `#RRGGBB` color strings.
- [ ] The app calls `load_app_color_scheme(app_id)` at startup.
- [ ] The app calls `activate_app_color_scheme(app_id)` before rendering.
- [ ] The app settings UI includes `default` as the global-system-theme option.
- [ ] The app is installed with `horizon_install_app` so assets are copied for development and production.

## Current example

`text-editor` is the reference implementation. It provides:

```text
apps/text-editor/assets/color-scheme.json
```

with these variants:

- `tokyo-night`
- `monokai`
- `solar-light`
- `dracula`
- `nord`
- `onedark`
- `gruvbox`
