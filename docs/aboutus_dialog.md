# AboutUs Dialog System

The **AboutUs Dialog** is a standardized way to present information about an application in the Horizon framework. It provides a structured, tabbed interface to show details like the application version, developers, thanks, translation credits, and more.

## Architecture

The AboutUs system relies on two main components within `horizon::AboutUsDialog`:
1. `AboutDialogContent`: A struct containing metadata (title, version, icon) and optional content widgets for different predefined tabs.
2. The `Application` API (`set_aboutus_content` and `show_aboutus`), which manages the dialog lifecycle implicitly.

### Predefined Tabs

The `AboutUsDialog` expects properties in `AboutDialogContent` to map to predefined tabs. **Not all of these tabs are mandatory**. Only the sections where you provide a non-null `std::unique_ptr<Widget>` will be rendered as a tab in the final dialog.

The available tabs correspond to these fields in `AboutDialogContent`:
- `about`: An overall "About" summary (e.g., Description of the app).
- `components`: Used to list external components or dependencies.
- `auths`: Authors and developers of the software.
- `thanks`: Special thanks or acknowledgments.
- `translate`: Translators who localized the app.

In addition to the tabs, you must define:
- `title`: The application title.
- `version`: The current version string.
- `icon`: The icon string (standard icon name).

## Usage Example

Below is an example of how to implement the `AboutUs` dialog in an application built on `horizon::Application`, taken from the `Terminal` project:

```cpp
#include "horizon/Application.hpp"
#include "horizon/dialogs/AboutUsDialog.hpp"
#include "horizon/Label.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/I18n.hpp"

// Inside your main() or Application setup:
app.set_aboutus_content([]() {
    // 1. Create content for the 'About' tab
    auto content_about = std::make_unique<Widget>();
    content_about->set_margin(15);
    content_about->set_spacing(15);

    auto lbl_about = std::make_unique<Label>();
    lbl_about->set_text(horizon::i18n().tr("terminal.aboutus.about"));
    lbl_about->set_vertical_alignment(VerticalAlignment::Top);

    content_about->add_child(std::move(lbl_about));
    content_about->add_child(Spacer());

    // 2. Create content for the 'Translations' tab
    auto content_translate = std::make_unique<Widget>();
    content_translate->set_spacing(15);
    content_translate->set_margin(15);

    auto lbl_translate = std::make_unique<Label>();
    lbl_translate->set_text(horizon::i18n().tr("terminal.aboutus.translate"));
    lbl_translate->set_vertical_alignment(VerticalAlignment::Top);

    content_translate->add_child(std::move(lbl_translate));
    content_translate->add_child(Spacer());

    // 3. Assemble the AboutDialogContent struct
    auto abus_content = std::make_unique<AboutDialogContent>();
    
    // Required properties
    abus_content->title = "Horizon Terminal";
    abus_content->version = "0.1.0";
    abus_content->icon = "utilities-terminal";
    
    // Assign mapped tabs. Leaving out 'components', 'auths', and 'thanks' 
    // will just omit those tabs from rendering.
    abus_content->about = std::move(content_about);
    abus_content->translate = std::move(content_translate);

    return abus_content;
});
```

Once the factory is set using `set_aboutus_content()`, the framework will handle instantiating the content each time the dialog is requested via the global menu's **About us** action, or by calling `WaylandWindow::show_aboutus()` or `Application::show_aboutus()`.

## Triggering the Dialog

Typically, invoking the dialog is handled automatically if the user selects the corresponding "About" item in the global system menu. Under the hood, this emits the `aboutus` signal, which the `WaylandWindow` automatically routes to call the `show_aboutus()` method.
