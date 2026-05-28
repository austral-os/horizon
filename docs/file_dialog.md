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

### Signals (EventsManager)
*   `when_accepted`: `EventsManager<FileDialogAcceptedContext>`. Executed when the user confirms the action (clicking Open/Save or pressing Enter in the text field). The context contains the `selected_path` property.
*   `when_cancelled`: `EventsManager<FileDialogCancelledContext>`. Executed when the user closes the dialog or clicks "Cancel".

## 4. File Filtering (FileFilters)

The selection dialog allows you to restrict file visibility (except for directories, which are always shown) by defining filters based on extensions or patterns.

To enable this feature, you need to declare an array of `horizon::FileFilter` structures and pass it to the dialog:

```cpp
std::vector<horizon::FileFilter> filters = {
    {"Images", {"*.png", "*.jpg", "*.jpeg"}},
    {"PDF Documents", {"*.pdf"}},
    {"All Supported Files", {"*.png", "*.jpg", "*.jpeg", "*.pdf"}},
    {"All Files", {"*"}}
};

// You must inject the list of filters before running the dialog
dialog->set_filters(filters);
```

When filters are injected, the dialog interface will automatically display a drop-down menu (Combo box) at the bottom. By selecting an option from the menu, the current view will dynamically refresh, hiding all files that do not match the `glob` patterns or extensions of the active filter.

## 5. Implementation Details and Navigation

*   **Sidebar**: Provides quick access to system folders like **All My Files**, **Applications**, **Desktop**, **Documents**, **Downloads**, and **iCloud Drive**.
*   **Search**: The toolbar includes a real-time filtered search field.
*   **View Modes**: The user can toggle between **Icon (Grid)**, **List**, and **CoverFlow** views using the toolbar controls.
*   **Navigation**: Supports backward and forward navigation, similar to a web browser.

## 6. Considerations for Save Mode

In `Save` or `SaveAs` mode, the dialog allows the user to type a file name that does not yet exist in the current directory. The `when_accepted` signal will return the full path constructed from the current directory and the entered name.

---
> [!TIP]
> Since `FileDialog` is a `WaylandWindow`, it behaves like an independent window. Ensure you properly manage the object's lifecycle (for example, keeping it in a `std::unique_ptr` within your main class) to prevent it from being destroyed before receiving the response.
