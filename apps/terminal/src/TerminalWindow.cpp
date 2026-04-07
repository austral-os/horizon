#include "TerminalWindow.hpp"
#include <memory>

namespace horizon {
namespace terminal {

TerminalWindow::TerminalWindow()
    : ApplicationWindow("Terminal") {
    
    // Create the terminal widget
    auto terminal = std::make_unique<TerminalWidget>();
    m_terminal = terminal.get();
    
    // Set it as the content of the ApplicationWindow
    // This will place it below the toolbar and above the status bar
    set_content(std::move(terminal));
}

} // namespace terminal
} // namespace horizon
