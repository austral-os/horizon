#include "CaptureWindow.hpp"
#include "CapturePreferences.hpp"
#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <iostream>
#include <memory>
#include <cstdlib>

using namespace horizon;
using namespace horizon::capture;

class CliApplication : public horizon::Application {
public:
    CliApplication(const std::string &app_id) 
        : Application(app_id, 1, 1, false, true) 
    {
        // By using a layer window instead of a standard window, the compositor 
        // won't draw any shadows or window frames around it.
        create_layer_window("org.horizon.capture.cli", 0, -1);
    }
};

int main(int argc, char** argv) {
    bool cli_mode = false;
    bool edit_mode = false;
    std::string mode = "";
    std::string type = "";
    
    // Check for CLI arguments first so we know which Application type to create
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--image") type = "image";
            else if (arg == "--video") type = "video";
            else if (arg == "--area") mode = "area";
            else if (arg == "--screen") mode = "screen";
            else if (arg == "--edit") edit_mode = true;
        }
        if (!type.empty() && !mode.empty()) {
            cli_mode = true;
        }
    }

    // Standard Horizon application setup
    std::unique_ptr<horizon::Application> app_ptr;
    if (cli_mode) {
        app_ptr = std::make_unique<CliApplication>("org.horizon.capture");
    } else {
        app_ptr = std::make_unique<horizon::Application>("org.horizon.capture", 600, 400);
    }
    Application& app = *app_ptr;
    
    // Load translations
    horizon::i18n().load_app_locales("capture");
    
    app.set_name(horizon::i18n().tr("capture.title"));
    app.set_icon_name("screenrecorder");

    // Setup preferences factory
    char *home = std::getenv("HOME");
    std::string config_path =
        home ? std::string(home) + "/.config/horizon/capture.json" : "capture.json";

    app.set_preferences_content(
        [config_path]()
        {
            auto content = std::make_unique<PreferencesContent>(config_path);
            auto *content_ptr = content.get();
            auto on_change = [content_ptr]() { content_ptr->save_config(); };

            content->add_section(horizon::i18n().tr("capture.preferences.general"), "preferences-system",
                                 std::make_unique<CaptureGeneralSection>(on_change), "general");
            content->add_section(horizon::i18n().tr("capture.preferences.image"), "camera-photo",
                                 std::make_unique<CaptureImageSection>(on_change), "image");
            content->add_section(horizon::i18n().tr("capture.preferences.video"), "camera-video",
                                 std::make_unique<CaptureVideoSection>(on_change), "video");

            return content;
        },
        800, 600);

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(horizon::i18n().tr("capture.title"));
    about.set_app_description(horizon::i18n().tr("capture.about.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("screenrecorder");
    about.add_app_author("Austral OS Team", "https://github.com/austral-os", "contact@austral-os.org");
    about.set_app_web("https://github.com/austral-os/horizon");

    auto window = std::make_unique<CaptureWindow>();
    auto* win_ptr = window.get();

    // App Menu (Global) - Horizon automatically adds Preferences, About and Quit
    app.set_app_menu(std::make_unique<Menu>());

    // Image Menu
    auto image_menu = std::make_unique<Menu>();
    image_menu->set_title(horizon::i18n().tr("capture.menu.image"));
    image_menu->add_item(horizon::i18n().tr("capture.menu.selection"), "Shift+S", "img_selection")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        LOG_INFO << "[CaptureApp] Menu: Image -> Selection clicked";
        win_ptr->capture_selection_image();
    });
    image_menu->add_item(horizon::i18n().tr("capture.menu.screen"), "Print", "img_screen")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        LOG_INFO << "[CaptureApp] Menu: Image -> Screen clicked";
        win_ptr->capture_screen_image();
    });
    app.add_menu(std::move(image_menu));
    
    // Video Menu
    auto video_menu = std::make_unique<Menu>();
    video_menu->set_title(horizon::i18n().tr("capture.menu.video"));
    video_menu->add_item(horizon::i18n().tr("capture.menu.selection"), "Ctrl+Shift+S", "vid_selection")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        LOG_INFO << "[CaptureApp] Menu: Video -> Selection clicked";
        win_ptr->start_selection_video();
    });
    video_menu->add_item(horizon::i18n().tr("capture.menu.screen"), "Ctrl+Print", "vid_screen")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        LOG_INFO << "[CaptureApp] Menu: Video -> Screen clicked";
        win_ptr->start_screen_video();
    });
    app.add_menu(std::move(video_menu));

    if (cli_mode) {
        win_ptr->when_application_load.connect([win_ptr, type, mode, edit_mode](horizon::EventContext&) {
            LOG_INFO << "[CaptureApp] when_application_load triggered for CLI action";
            
            win_ptr->set_quit_on_finish(true);
            win_ptr->set_edit_on_finish(edit_mode);
            
            if (win_ptr->application()) {
                win_ptr->application()->post_task([win_ptr, type, mode]() {
                    if (type == "image") {
                        if (mode == "area") {
                            win_ptr->capture_selection_image();
                        } else if (mode == "screen") {
                            win_ptr->capture_screen_image();
                        }
                    } else if (type == "video") {
                        if (mode == "area") {
                            win_ptr->start_selection_video();
                        } else if (mode == "screen") {
                            win_ptr->start_screen_video();
                        }
                    }
                });
            }
        });
    }

    app.set_root(std::move(window));

    app.run();
    
    return 0;
}
