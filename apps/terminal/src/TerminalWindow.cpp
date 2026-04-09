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
    set_content(std::move(terminal));

    m_terminal->set_focus(true);
}

} // namespace terminal
} // namespace horizon
