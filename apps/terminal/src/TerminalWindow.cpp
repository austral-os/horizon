#include "TerminalWindow.hpp"
#include "TerminalWidget.hpp"
#include "TerminalColorScheme.hpp"
#include "horizon/I18n.hpp"
#include "horizon/Logger.hpp"
#include <memory>

namespace horizon {
namespace terminal {

TerminalWindow::TerminalWindow()
    : ApplicationWindow(i18n().tr("terminal.title")) {
    
    set_draw_background(false);
    
    // 0. Load global color scheme from config
    TerminalConfig config = ConfigReader::load();
    m_scheme = config.theme;
    if (application()) {
        application()->set_blur(config.blur);
    }

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
        if (!application()) return;

        LOG_INFO << "[TERMINAL] Fullscreen toggle clicked";
        if (application()->is_fullscreen()) {
            application()->unfullscreen();
        } else {
            // Ensure we have a focused terminal to target
            if (m_tabs->current_tab_body()) {
                m_tabs->current_tab_body()->set_focus(true);
            }
            application()->fullscreen();
        }
    });

    m_toolbar->when_preferences_clicked.connect([this](PreferencesClickEvent &) {
        LOG_INFO << "[TERMINAL] Preferences clicked";
        if (application()) {
            application()->show_preferences();
        }
    });
}

void TerminalWindow::create_new_tab() {
    LOG_INFO << "[TERMINAL] Creating new tab";
    
    auto terminal = std::make_unique<TerminalWidget>();
    auto* ptr = terminal.get();
    
    // Aplicar esquema de color cargado
    ptr->set_color_scheme(m_scheme);

    // Initialize the terminal shell
    ptr->spawn();

    // Sincronizar el modo inmersivo con las señales de pantalla completa del widget
    ptr->when_enter_fullscreen.connect([this](FullscreenEventContext &) {
        this->set_immersive_mode(true);
    });

    ptr->when_leave_fullscreen.connect([this](FullscreenEventContext &) {
        this->set_immersive_mode(false);
    });

    m_tabs->add_tab(i18n().tr("terminal.title"), std::move(terminal));
    
    // Focus the newly created terminal (deferred if window is ready, direct if still initializing)
    auto* app = application();
    if (app) {
        app->post_task([ptr]() {
            ptr->set_focus(true);
        });
    } else {
        ptr->set_focus(true);
    }
}

} // namespace terminal
} // namespace horizon
