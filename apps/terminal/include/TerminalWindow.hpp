#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "TerminalWidget.hpp"
#include <memory>
#include <string>

namespace horizon {
namespace terminal {

class TerminalWindow : public horizon::ApplicationWindow {
public:
    TerminalWindow();
    ~TerminalWindow() = default;

private:
    TerminalWidget* m_terminal;
};

} // namespace terminal
} // namespace horizon
