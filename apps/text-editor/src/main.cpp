#include "TextEditorWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char** argv) {
    horizon::Application app("org.horizon.text-editor");
    
    // Set up i18n
    horizon::i18n().load_locale("es", "/usr/share/horizon/locales"); // Fallback to Spanish core
    
    auto window = std::make_unique<horizon::text_editor::TextEditorWindow>();
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
