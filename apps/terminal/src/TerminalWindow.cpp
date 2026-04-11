#include "TerminalWindow.hpp"
#include "TerminalWidget.hpp"
#include "horizon/I18n.hpp"
#include "horizon/Logger.hpp"
#include <memory>

namespace horizon {
namespace terminal {

TerminalWindow::TerminalWindow()
    : ApplicationWindow(i18n().tr("terminal.title")) {
    
    // 1. Terminal Toolbar
    auto terminal_toolbar = std::make_unique<TerminalToolbar>();
    m_toolbar = terminal_toolbar.get();
    toolbar()->add_toolbar_widget(std::move(terminal_toolbar));

    // 2. Tab Collection
    auto tabs = std::make_unique<TabCollection>();
    m_tabs = tabs.get();
    m_tabs->set_smart_header(true);
    m_tabs->set_closable_tabs(true);

    m_tabs->when_add_tab_clicked.connect([this](EventContext &) {
        this->create_new_tab();
    });

    m_tabs->when_tab_close_requested.connect([this](int index) {
        application()->post_task([this, index]() {
            m_tabs->remove_tab(index);
            if (m_tabs->tab_count() == 0) {
                application()->quit();
            }
        });
    });
    
    // Set it as the content of the ApplicationWindow
    set_content(std::move(tabs));

    // 3. Initial tab
    create_new_tab();

    // --- Connections ---
    m_toolbar->when_new_tab_clicked.connect([this](NewTabClickEvent &) {
        this->create_new_tab();
    });

    m_toolbar->when_fullscreen_clicked.connect([this](FullscreenClickEvent &) {
        LOG_INFO << "[TERMINAL] Fullscreen clicked";
        set_immersive_mode(!m_is_immersive);
    });

    m_toolbar->when_preferences_clicked.connect([this](PreferencesClickEvent &) {
        LOG_INFO << "[TERMINAL] Preferences clicked";
    });
}

void TerminalWindow::create_new_tab() {
    LOG_INFO << "[TERMINAL] Creating new tab";
    
    auto terminal = std::make_unique<TerminalWidget>();
    auto* ptr = terminal.get();
    
    // Initialize the terminal shell
    ptr->spawn();

    m_tabs->add_tab(i18n().tr("terminal.title"), std::move(terminal));
    
    // Focus the newly created terminal
    ptr->set_focus(true);
}

} // namespace terminal
} // namespace horizon
