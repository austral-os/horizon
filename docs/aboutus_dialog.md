# About System in Horizon

The **About System** is a centralized and mandatory mechanism in the Horizon framework to ensure every application provides consistent metadata and branding. It replaces the legacy `AboutUsFactory` and `AboutDialogContent` with a robust `AboutManager` integrated directly into the `Application` base class.

## Architecture

The system is built around three core pillars:

1.  **`About` Struct**: A data structure defined in `horizon/About.hpp` that holds metadata like title, version, icon, description, website, and lists of authors and translators.
2.  **`AboutManager`**: A class that manages two sets of `About` data:
    *   **Horizon Data**: Static information about the Horizon Desktop Environment (always present).
    *   **Application Data**: Information specific to the current application (configurable by the developer).
3.  **`AboutUsDialog`**: A standardized, premium UI component that displays this information in a tabbed notebook with two fixed sections: **Application** and **Horizon**.

## Mandatory Metadata

To ensure a high-quality user experience, Horizon enforces the presence of specific metadata. If any of these fields are missing, the application will fail to start during `Application::run()` with a descriptive error.

**Mandatory fields for every application:**
*   **Title**: The display name of the application.
*   **Version**: Semantic version string (e.g., "0.1.0").
*   **Description**: A brief summary of what the application does.
*   **Icon**: The name of the icon in the system theme.
*   **Git Repository**: The URL to the source code.

## Usage

Metadata is configured using the `about_manager()` provided by the `Application` instance, typically in the `main()` function or the application constructor.

### Basic Configuration Example

```cpp
#include <horizon/Application.hpp>
#include <horizon/About.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.horizon.myapp", 800, 600);

    // Access the about manager to configure app info
    auto &about = app.about_manager();
    
    about.set_app_title("My Awesome App");
    about.set_app_description("This application solves complex problems with ease.");
    about.set_app_version("1.0.0");
    about.set_app_icon("my-app-icon");
    about.set_app_git("https://github.com/user/my-app");

    // Add optional information
    about.add_app_author("John Doe", "https://johndoe.com", "john@example.com");
    about.add_app_translator("Jane Smith", "", "jane@example.com");

    // ... setup windows ...

    return app.run(); // Validation happens here
}
```

## Features of the About Dialog

The new `AboutUsDialog` provides several automatic features:

*   **Dynamic Layout**: Automatically calculates the required height for content to ensure perfect scrolling.
*   **Premium Aesthetics**: Includes background blur and high-quality typography.
*   **Horizon Branding**: Every application automatically includes a "Horizon" tab with the system icon, title, version, and a link to the project, fostering ecosystem identity.
*   **Automatic Integration**: The "About" item in the global menu is automatically wired to show this dialog using the configured manager.

## Troubleshooting

### "Mandatory metadata missing" error
If you see this error in the console, check that you have called all `set_app_*` methods in the `AboutManager` before calling `app.run()`.

### "show_aboutus(legacy) called" warning
This warning indicates that a window is trying to show the about dialog without a manager. Ensure you are using the `Application` class to create your windows, as it automatically wires the `AboutManager` to every window it manages.
