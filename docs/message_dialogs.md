# MessageDialog System

Horizon provides a standardized way to display simple message dialogs, such as alerts and confirmations. These dialogs are integrated into the core `WaylandWindow` class, making them accessible from any window or widget in the framework.


## Overview

The `MessageDialog` system supports four types of messages, each with its own visual style and icon:

- **Info**: General information messages.
- **Warning**: Important warnings that require user attention.
- **Error**: Error messages or critical failures.
- **Question**: Requests for user input or confirmation.

## Basic Usage

The easiest way to show dialogs is using the `WaylandWindow` methods. Since every `Widget` has access to its parent window via the `application()` method, you can trigger dialogs from anywhere in your UI.


### Alerts

An alert is a non-blocking dialog (from the perspective of the application's internal tasks, though it's technically a child window) that displays a message and an "Accept" button.

```cpp
#include <horizon/WaylandWindow.hpp>

// ... inside a Widget's event handler
application()->alert("This is an information message.", "Information", MessageType::Info);
```

**Signature:**
```cpp
void WaylandWindow::alert(const std::string &message, 
                         const std::string &title = "Alert", 
                         MessageType type = MessageType::Info);
```


### Confirmations

A confirmation dialog allows the user to choice between "Accept" and "Cancel". The `confirm` method is **blocking**; it waits for the user to respond and returns a boolean.

```cpp
#include <horizon/WaylandWindow.hpp>

// ... inside a Widget's event handler
if (application()->confirm("Are you sure you want to delete this file?", "Confirm Delete")) {
    // User clicked Accept
    delete_file();
}
```

**Signature:**
```cpp
bool WaylandWindow::confirm(const std::string &message, 
                           const std::string &title = "Confirm", 
                           MessageType type = MessageType::Question);
```


> [!WARNING]
> Since `confirm()` is blocking, it should only be called when you want to stop the current execution flow until the user responds.

---

## Advanced Usage: `MessageDialog` Class

For more control, you can instantiate `MessageDialog` directly. This allows you to customize button text and connect to the `when_responded` signal asynchronously.

### Manual Initialization

```cpp
#include <horizon/MessageDialog.hpp>

auto dialog = std::make_unique<MessageDialog>("Custom Dialog", "Do you like this toolkit?", MessageType::Question, true);

// Customize button text
dialog->set_accept_text("Yes!");
dialog->set_cancel_text("Not yet");

// Connect to responses asynchronously
dialog->when_responded.connect([](MessageResponseEvent res) {
    if (res.response == MessageResponse::Accept) {
        // ...
    }
});

// The dialog must be managed by the Application to run its own event loop thread
// (This is usually handled automatically by Application::alert/confirm)
```

## Reference

These enums are defined in `<horizon/DialogTypes.hpp>` and are shared across the framework to avoid circular dependencies.

### `MessageType`
```cpp
enum class MessageType {
    Info,
    Warning,
    Error,
    Question
};
```

### `MessageResponse`
```cpp
enum class MessageResponse {
    Accept,
    Cancel
};
```

