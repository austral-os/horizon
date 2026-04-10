#include "horizon/Application.hpp"
#include "TerminalWindow.hpp"
#include "horizon/I18n.hpp"

using namespace horizon;
using namespace horizon::terminal;

int main(int argc, char** argv) {
    Application app("org.horizon.terminal", 800, 600);
    
    // Load translations
    horizon::i18n().load_app_locales("terminal");

    app.set_name(horizon::i18n().tr("terminal.title"));
    app.set_icon_name("utilities-terminal");
    
    // Create the terminal window (properly decorators with Horizon window frame)
    auto terminal_window = std::make_unique<TerminalWindow>();
    
    // Set it as the main root of the application window
    app.set_root(std::move(terminal_window));
    
    // Run the main app loop
    app.run();
    
    return 0;
}
