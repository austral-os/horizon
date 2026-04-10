#include "TerminalWindow.hpp"
#include "horizon/I18n.hpp"
#include <memory>

namespace horizon {
namespace terminal {

TerminalWindow::TerminalWindow()
    : ApplicationWindow(i18n().tr("terminal.title")) {
    
    // Create the terminal widget
    auto terminal = std::make_unique<TerminalWidget>();
    m_terminal = terminal.get();
    
    // Set it as the content of the ApplicationWindow
    set_content(std::move(terminal));

    m_terminal->set_focus(true);
}

} // namespace terminal
} // namespace horizon
