#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "TerminalToolbar.hpp"
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
};

} // namespace terminal
} // namespace horizon
