#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "TerminalToolbar.hpp"
#include "TerminalColorScheme.hpp"
#include <horizon/TabCollection.hpp>

namespace horizon {
namespace terminal {

class TerminalWindow : public horizon::ApplicationWindow {
public:
    TerminalWindow();
    ~TerminalWindow() = default;

    void create_new_tab();

private:
    TabCollection* m_tabs;
    TerminalToolbar* m_toolbar;
    TerminalColorScheme m_scheme;
};

} // namespace terminal
} // namespace horizon
