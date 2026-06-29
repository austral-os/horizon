# `FileDialog` Documentation

The Horizon framework provides the `FileDialog` class to facilitate the selection of files and folders within applications. This component inherits from `WaylandWindow` and offers a standardized interface that includes a places sidebar, a file view (grid or list), a toolbar with search, and navigation controls.

## 1. Operating Modes

When instantiating a `FileDialog`, you must specify the operating mode using the `FileDialogMode` enum. The available modes are:

| Mode | Behavior |
| :--- | :--- |
| `FileDialogMode::Open` | Optimized for opening existing files. The main button displays "Open". Double-clicking a file accepts the selection immediately. |
| `FileDialogMode::Save` | Optimized for saving files. The main button displays "Save" and the text field label changes to "Save as:". |
| `FileDialogMode::SaveAs` | Similar to `Save`. (Currently shares most of the visual logic with `Save`). |
| `FileDialogMode::SelectFolder` | Designed for selecting directories. |
| `FileDialogMode::New` | Used for creating new files or projects. |

## 2. Basic Usage

To use a file dialog, you must create an instance, set up the response callbacks, and display the window.

### Example: Opening a file

```cpp
#include <horizon/dialogs/FileDialog/FileDialog.hpp>

// ... inside a method of your application

auto dialog = std::make_unique<horizon::FileDialog>(
    horizon::FileDialogMode::Open, 
    "Open Document"
);

// Configure what to do when the user selects a file
dialog->when_accepted.connect([](horizon::FileDialogAcceptedContext &ctx) {
    LOG_INFO("Selected file: {}", ctx.selected_path);
    // Logic to open the file
});

// Configure what to do if the user cancels
dialog->when_cancelled.connect([](horizon::FileDialogCancelledContext &ctx) {
    LOG_INFO("Selection cancelled");
});

// Set the initial path (optional)
dialog->set_current_path("/home/user/Documents");

// The dialog is automatically displayed as it is handled by the window system,
// or it can be added to the application.
```

## 3. Public API

### Constructor
```cpp
FileDialog(FileDialogMode mode, const std::string &title = "");
```
*   `mode`: The operating mode (`Open`, `Save`, `SaveAs`, `SelectFolder` or `New`).
*   `title`: The title to be displayed in the window's title bar. If left empty, a default value based on the mode will be used.

### Methods
*   `void set_current_path(const std::string &path)`: Changes the current directory shown in the dialog.
*   `std::string selected_path() const`: Returns the full path currently entered or selected in the dialog.
*   `void set_select_multiple(bool select_multiple)`: Enables multiple file selection for `Open` mode. This is ignored by save-oriented flows.
*   `bool select_multiple() const`: Returns whether multiple selection is enabled.

### Signals (EventsManager)
*   `when_accepted`: `EventsManager<FileDialogAcceptedContext>`. Executed when the user confirms the action (clicking Open/Save or pressing Enter in the text field). The context contains `selected_path` for backward compatibility and `selected_paths` for all accepted paths.
*   `when_cancelled`: `EventsManager<FileDialogCancelledContext>`. Executed when the user closes the dialog or clicks "Cancel".

### Accepted context

| Property | Meaning |
| :--- | :--- |
| `selected_path` | The accepted path. In multiple-selection mode, this is the first selected path. |
| `selected_paths` | All accepted paths. In single-selection mode, this contains one item. |

`selected_path` remains available so existing applications do not need to change when they only support one file.

## 4. Multiple Selection

Multiple selection is supported for `FileDialogMode::Open`:

```cpp
auto dialog = std::make_unique<horizon::FileDialog>(
    horizon::FileDialogMode::Open,
    "Open Documents"
);

dialog->set_select_multiple(true);
dialog->when_accepted.connect([](horizon::FileDialogAcceptedContext &ctx) {
    for (const auto &path : ctx.selected_paths) {
        // Open each selected file.
    }
});
```

Only regular files are returned from an open-file multiple selection. Directories continue to behave as navigation targets.

When Horizon is used as an XDG Desktop Portal backend, `xdg-desktop-portal-horizon` maps the standard `org.freedesktop.portal.FileChooser.OpenFile` `multiple` option to this mode and returns all selected files in the portal `uris` result.

## 5. File Filtering (`FileFilter`)

The selection dialog allows you to restrict file visibility (except for directories, which are always shown) by defining filters based on extensions or patterns.

### Direct dialog usage

To enable filtering on a manually created `FileDialog`, declare a list of `horizon::FileFilter` structures and pass it to the dialog before calling `run()`:

```cpp
std::vector<horizon::FileFilter> filters = {
    {"Images", {"*.png", "*.jpg", "*.jpeg"}},
    {"PDF Documents", {"*.pdf"}},
    {"All Supported Files", {"*.png", "*.jpg", "*.jpeg", "*.pdf"}},
    {"All Files", {"*"}}
};

dialog->set_filters(filters);
```

When filters are injected, the dialog interface will automatically display a drop-down menu (Combo box) at the bottom. By selecting an option from the menu, the current view will dynamically refresh, hiding all files that do not match the `glob` patterns or extensions of the active filter.

### Window-level filters for application actions

Applications that use the standard `file.open`, `file.save`, or `file.save_as` actions should expose supported formats once from their `Window` subclass:

```cpp
#include <horizon/dialogs/FileFilter.hpp>

std::vector<horizon::FileFilter> MyWindow::file_filters() const
{
    return {
        {"Text Files", {"*.txt"}, horizon::FileFilterUsage::All},
        {"Markdown Files", {"*.md"}, horizon::FileFilterUsage::All},
        {"All Files", {"*"}, horizon::FileFilterUsage::All}
    };
}
```

The framework applies these filters automatically when it creates the standard file dialog:

| Usage | Dialogs that receive the filter |
| :--- | :--- |
| `FileFilterUsage::All` | `Open`, `Save`, and `SaveAs` |
| `FileFilterUsage::Open` | `Open` only |
| `FileFilterUsage::Save` | `Save` and `SaveAs` only |

If `usage` is omitted, it defaults to `FileFilterUsage::All`, so existing two-field initializers remain valid:

```cpp
{"Images", {"*.png", "*.jpg"}} // Applies to all dialog modes.
```

## 6. Implementation Details and Navigation

*   **Sidebar**: Provides quick access to system folders like **All My Files**, **Applications**, **Desktop**, **Documents**, **Downloads**, and **iCloud Drive**.
*   **Search**: The toolbar includes a real-time filtered search field.
*   **View Modes**: The user can toggle between **Icon (Grid)**, **List**, and **CoverFlow** views using the toolbar controls.
*   **Navigation**: Supports backward and forward navigation, similar to a web browser.

## 7. Considerations for Save Mode

In `Save` or `SaveAs` mode, the dialog allows the user to type a file name that does not yet exist in the current directory. The `when_accepted` signal will return the full path constructed from the current directory and the entered name.

When the active filter has a single concrete extension pattern such as `*.txt` or `*.md`, saving an extensionless filename appends that extension automatically. For example, entering `notes` with a `*.md` filter returns `notes.md`.

The `All Files` filter should use the `*` pattern. It does not append or enforce any extension, so users can type any filename they need.

Selecting an existing regular file in `Save` or `SaveAs` mode fills the filename text field without accepting the dialog. This supports overwrite flows: users can click an existing file to copy its name, then explicitly confirm with **Save**.

---
> [!TIP]
> Since `FileDialog` is a `WaylandWindow`, it behaves like an independent window. Ensure you properly manage the object's lifecycle (for example, keeping it in a `std::unique_ptr` within your main class) to prevent it from being destroyed before receiving the response.
