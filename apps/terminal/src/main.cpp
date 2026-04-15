#include "TerminalGeneralSection.hpp"
#include "TerminalWindow.hpp"
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
            auto content_about = std::make_unique<Widget>();
            content_about->set_margin(15);
            content_about->set_spacing(15);

            auto lbl_about = std::make_unique<Label>();
            lbl_about->set_text(horizon::i18n().tr("terminal.aboutus.about"));
            lbl_about->set_vertical_alignment(VerticalAlignment::Top);
            lbl_about->set_fixed_size(25);

            auto link_horizon = std::make_unique<Link>();
            link_horizon->set_text("https://github.com/austral-os/horizon");
            link_horizon->set_url("https://github.com/austral-os/horizon");
            link_horizon->set_fixed_size(25);

            content_about->add_child(std::move(lbl_about));
            content_about->add_child(std::move(link_horizon));
            content_about->add_child(Spacer());

            auto content_translate = std::make_unique<Widget>();
            content_translate->set_spacing(15);
            content_translate->set_margin(15);

            auto lbl_translate = std::make_unique<Label>();
            lbl_translate->set_text(horizon::i18n().tr("terminal.aboutus.translate"));
            lbl_translate->set_vertical_alignment(VerticalAlignment::Top);

            content_translate->add_child(std::move(lbl_translate));
            content_translate->add_child(Spacer());

            auto abus_content = std::make_unique<AboutDialogContent>();
            abus_content->title = "Horizon Terminal";
            abus_content->version = "0.1.0";
            abus_content->icon = "horizon-terminal";
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
