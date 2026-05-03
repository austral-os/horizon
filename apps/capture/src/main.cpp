#include "CaptureWindow.hpp"
#include "CapturePreferences.hpp"
#include <horizon/Application.hpp>
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

int main(int argc, char** argv) {
    // Standard Horizon application setup
    Application app("org.horizon.capture", 600, 400);
    app.set_name("Capture");
    app.set_icon_name("camera-photo");

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

            content->add_section("General", "preferences-system",
                                 std::make_unique<CaptureGeneralSection>(on_change), "general");
            content->add_section("Image", "camera-photo",
                                 std::make_unique<CaptureImageSection>(on_change), "image");
            content->add_section("Video", "camera-video",
                                 std::make_unique<CaptureVideoSection>(on_change), "video");

            return content;
        },
        800, 600);

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Horizon Capture");
    about.set_app_description("A professional screen capture and recording utility for Austral OS.");
    about.set_app_version("1.0.0");
    about.set_app_icon("camera-photo");
    about.add_app_author("Austral OS Team", "https://github.com/austral-os", "contact@austral-os.org");
    about.set_app_web("https://github.com/austral-os/horizon");

    auto window = std::make_unique<CaptureWindow>();
    auto* win_ptr = window.get();

    // App Menu (Global)
    auto app_menu = std::make_unique<Menu>();
    app_menu->add_item("Preferences", "Ctrl+P", "preferences")->when_click.connect([&app](const MouseButtonEventContext&) {
        app.show_preferences();
    });
    app_menu->add_item("About", "", "about")->when_click.connect([&app](const MouseButtonEventContext&) {
        app.show_aboutus();
    });
    app_menu->add_separator();
    app_menu->add_item("Quit", "Ctrl+Q", "quit")->when_click.connect([&app](const MouseButtonEventContext&) {
        exit(0);
    });
    app.set_app_menu(std::move(app_menu));

    // Image Menu
    auto image_menu = std::make_unique<Menu>();
    image_menu->set_title("Image");
    image_menu->add_item("Selection", "Shift+S", "img_selection")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->capture_selection_image();
    });
    image_menu->add_item("Window", "Shift+W", "img_window")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->capture_window_image();
    });
    image_menu->add_item("Full Screen", "Print", "img_screen")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->capture_screen_image();
    });
    app.add_menu(std::move(image_menu));

    // Video Menu
    auto video_menu = std::make_unique<Menu>();
    video_menu->set_title("Video");
    video_menu->add_item("Selection", "Ctrl+Shift+S", "vid_selection")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->start_selection_video();
    });
    video_menu->add_item("Window", "Ctrl+Shift+W", "vid_window")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->start_window_video();
    });
    video_menu->add_item("Full Screen", "Ctrl+Print", "vid_screen")->when_click.connect([win_ptr](const MouseButtonEventContext&) {
        win_ptr->start_screen_video();
    });
    app.add_menu(std::move(video_menu));

    app.set_root(std::move(window));
    
    // Check for CLI arguments (optional, can still support basic ones)
    if (argc > 1) {
        // ... CLI logic could be kept here if needed, but the user wants a GUI ...
    }

    app.run();
    
    return 0;
}
