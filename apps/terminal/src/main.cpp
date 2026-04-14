#include "TerminalGeneralSection.hpp"
#include "TerminalWindow.hpp"
#include "horizon/Application.hpp"
#include "horizon/I18n.hpp"
#include "horizon/dialogs/AboutUsDialog.hpp"
#include "horizon/dialogs/PreferencesContent.hpp"
#include <cstdlib>

using namespace horizon;
using namespace horizon::terminal;

int main(int argc, char **argv)
{
    Application app("org.horizon.terminal", 800, 600);

    // Load translations
    horizon::i18n().load_app_locales("terminal");

    app.set_name(horizon::i18n().tr("terminal.title"));
    app.set_icon_name("utilities-terminal");

    // Setup preferences factory on the application
    char *home = std::getenv("HOME");
    std::string config_path =
        home ? std::string(home) + "/.config/horizon/terminal.json" : "terminal.json";

    app.set_preferences_content(
        [config_path]()
        {
            auto content = std::make_unique<PreferencesContent>(config_path);
            auto *content_ptr = content.get();

            auto on_change = [content_ptr]() { content_ptr->save_config(); };

            // We use "terminal" as the section name to match the requested JSON structure
            content->add_section("General", "preferences-system",
                                 std::make_unique<TerminalGeneralSection>(on_change), "terminal");

            return content;
        },
        800, 600);

    app.set_aboutus_content(
        []()
        {
            auto content_about = std::make_unique<Label>();
            content_about->set_text("Este es el contenido");

            auto content_translate = std::make_unique<Label>();
            content_translate->set_text("Este es el contenido de traduccion");

            auto abus_content = std::make_unique<AboutDialogContent>();
            abus_content->title = "Horizon Terminal";
            abus_content->version = "0.1.0";
            abus_content->icon = "utilities-terminal";
            abus_content->about = std::move(content_about);
            abus_content->translate = std::move(content_translate);

            return abus_content;
        });

    // Create the terminal window (properly decorated with Horizon window frame)
    auto terminal_window = std::make_unique<TerminalWindow>();

    // Set it as the main root of the application window
    app.set_root(std::move(terminal_window));

    // Run the main app loop
    app.run();

    return 0;
}
