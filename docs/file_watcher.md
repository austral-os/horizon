# File Watcher System

The `FileWatcher` class provides a simple and efficient way to monitor filesystem changes in real-time. It leverages the Linux `inotify` API to detect file modifications, movements, and deletions, making it ideal for features like real-time configuration reloading.

## Key Features

- **Asynchronous Monitoring**: Runs in a dedicated background thread to prevent blocking the main application loop.
- **Event Debouncing**: Includes a built-in 200ms debounce mechanism to ensure the file is fully written before triggering a change event.
- **Main Thread Integration**: Provides a synchronization hook to safely execute reactions on the main application thread.

## API Reference

The `FileWatcher` is an abstract base class. To use it, you must inherit from it and implement the following protected methods:

### Methods

- **`void start_watching(const std::string& path)`**: Begins monitoring the specified file.
- **`void stop_watching()`**: Gracefully stops the monitoring thread and releases resources.

### Virtual Hooks

- **`virtual void on_file_changed() = 0`**: Called whenever a change is detected. This is where your reload logic should go.
- **`virtual void post_watcher_task(std::function<void()> task) = 0`**: Since `inotify` events are detected on a background thread, this method must be implemented to schedule the `on_file_changed` call on the main thread (usually via `WaylandWindow::post_task`).

---

## Simple Example

Here is a complete example of a simple application that reloads its configuration automatically when the file is saved.

```cpp
#include <horizon/WaylandWindow.hpp>
#include <horizon/FileWatcher.hpp>
#include <horizon/Logger.hpp>
#include <iostream>

/**
 * A window that watches its own configuration file.
 */
class ConfigMonitorApp : public horizon::WaylandWindow, public horizon::FileWatcher {
public:
    ConfigMonitorApp() : WaylandWindow("Config Monitor", 400, 300) {
        // Start watching a specific configuration file
        std::string config_path = "/tmp/app_settings.json";
        start_watching(config_path);
        
        LOG_INFO << "Modify " << config_path << " to see live updates!";
    }

protected:
    /**
     * This method will be executed on the MAIN THREAD thank to 
     * the implementation of post_watcher_task below.
     */
    void on_file_changed() override {
        LOG_INFO << "Change detected! Reloading settings...";
        // Add your logic to re-read the file here
        this->invalidate(); // Example: force a redraw
    }

    /**
     * Bridges the background watcher thread with the Wayland event loop.
     */
    void post_watcher_task(std::function<void()> task) override {
        // Uses WaylandWindow's thread-safe task queue
        this->post_task(task);
    }
};

int main() {
    ConfigMonitorApp app;
    return app.run();
}
```

## Integration in Horizon

By default, several Horizon components use this system:
- **Terminal**: Reloads fonts, colors, and scrollback settings when `terminal.json` is modified.
- **System Themes**: The framework can monitor theme files to apply visual changes across all running applications.

---

> [!IMPORTANT]
> Always ensure that `post_watcher_task` is correctly implemented. Directly modifying UI components or shared state from `on_file_changed` without thread synchronization will lead to crashes or undefined behavior.
