#include "TerminalColorSection.hpp"
#include "TerminalGeneralSection.hpp"
#include "TerminalWindow.hpp"
#include "horizon/About.hpp"
#include "horizon/Application.hpp"
#include "horizon/I18n.hpp"
#include "horizon/Link.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/dialogs/AboutUsDialog.hpp"
#include "horizon/dialogs/PreferencesContent.hpp"
#include <cstdlib>

using namespace horizon;
using namespace horizon::terminal;

int main(int argc, char **argv)
{
    Application app("org.horizon.terminal", 1390, 875);

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

            // We use "terminal" as the section name for both pages to consolidate settings
            content->add_section(horizon::i18n().tr("terminal.preferences.general_tab"),
                                 "preferences-system",
                                 std::make_unique<TerminalGeneralSection>(on_change), "terminal");

            auto color_section = std::make_unique<TerminalColorSection>(nullptr);
            auto *color_section_ptr = color_section.get();
            content->add_section(horizon::i18n().tr("terminal.preferences.colors_tab"),
                                 "preferences-desktop-color", std::move(color_section), "terminal");

            // --- Theme Integration (Root Level) ---
            // 1. Initial Load: Sync theme from root to the section UI
            if (content->config_data().contains("theme"))
            {
                color_section_ptr->set_current_theme(
                    TerminalColorScheme::from_json(content->config_data()["theme"]));
            }

            // 2. Setup save sync: Update root "theme" whenever any setting changes
            color_section_ptr->set_on_change(
                [content_ptr, color_section_ptr]()
                {
                    content_ptr->config_data()["theme"] =
                        color_section_ptr->get_current_theme().to_json();
                    content_ptr->save_config();
                });

            return content;
        },
        1390, 875);

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Horizon Terminal");
    about.set_app_description(horizon::i18n().tr("terminal.aboutus.about"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("horizon-terminal");
    about.set_app_git(ABOUT_HORIZON.git);
    about.add_app_translator("Horacio", "https://github.com/austral-os/horizon",
                             "horaciodrs@gmail.com");

    // Create the terminal window (properly decorated with Horizon window frame)
    auto terminal_window = std::make_unique<TerminalWindow>();

    // Set it as the main root of the application window
    app.set_root(std::move(terminal_window));

    // Run the main app loop
    app.run();

    return 0;
}
