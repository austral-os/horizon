#include "TextEditorWindow.hpp"
#include "TextEditorGeneralSection.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <cstdlib>

using namespace horizon;

int main(int argc, char** argv) {
    Application app("org.horizon.text-editor", 1024, 768);
    
    // Set up i18n
    i18n().load_app_locales("text-editor");
    app.set_name(i18n().tr("text_editor.title"));
    app.set_icon_name("accessories-text-editor");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Text Editor");
    about.set_app_description("A modern text editor for Horizon with syntax highlighting support.");
    about.set_app_version(APP_VERSION);
    about.set_app_icon("accessories-text-editor");

    // Setup preferences factory
    char *home = std::getenv("HOME");
    std::string config_path =
        home ? std::string(home) + "/.config/horizon/text-editor.json" : "text-editor.json";

    app.set_preferences_content(
        [config_path]()
        {
            auto content = std::make_unique<PreferencesContent>(config_path);
            auto *content_ptr = content.get();
            auto on_change = [content_ptr]() { content_ptr->save_config(); };

            content->add_section("General", "preferences-system",
                                 std::make_unique<text_editor::TextEditorGeneralSection>(on_change), "editor");

            return content;
        },
        800, 600);
    
    auto window = std::make_unique<text_editor::TextEditorWindow>();
    auto* window_ptr = window.get();
    
    // Check if a file was passed as argument
    if (argc > 1) {
        window_ptr->open_file(argv[1]);
    } else {
        window_ptr->new_file();
    }
    
    app.set_root(std::move(window));
    app.run();
    
    return 0;
}
