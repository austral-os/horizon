#include <horizon/WaylandWindow.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Logger.hpp>
#include <memory>

/**
 * Horizon I18n Demo
 * 
 * This example demonstrates:
 * 1. Automatic loading of app-specific locales.
 * 2. Automatic merging with core system locales.
 * 3. String interpolation ( {name} ).
 * 4. Pluralization.
 */

int main(int argc, char** argv) {
    // 1. Initialize Logger
    horizon::Logger::instance().init("i18n_demo");

    // 2. Setup Window
    horizon::WaylandWindow app("org.horizon.i18n_demo", 400, 300);

    // 3. Load translations for this app
    // This will look for 'locales/es.json' or 'locales/en.json'
    horizon::i18n().load_app_locales("i18n_demo");

    // 4. Use translated strings
    app.set_name(horizon::i18n().tr("i18n_demo.title"));

    auto root = std::make_unique<horizon::Widget>();
    root->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    root->set_margin(20);
    root->set_spacing(10);

    // Simple translation
    root->add_child(std::make_unique<horizon::Label>(
        horizon::i18n().tr("i18n_demo.welcome")
    ));

    // Interpolation demo
    root->add_child(std::make_unique<horizon::Label>(
        horizon::i18n().tr("i18n_demo.greeting", {{"user", "Horizon Developer"}})
    ));

    // Pluralization demo
    root->add_child(std::make_unique<horizon::Label>(
        horizon::i18n().tr("i18n_demo.messages_count", 1)
    ));
    root->add_child(std::make_unique<horizon::Label>(
        horizon::i18n().tr("i18n_demo.messages_count", 5)
    ));

    // Core library translation demo (automatically loaded)
    root->add_child(std::make_unique<horizon::Label>(
        "Core Accept: " + horizon::i18n().tr("core.dialog.accept")
    ));

    app.set_root(std::move(root));

    LOG_INFO << "Starting i18n demo application...";
    app.run();
    return 0;
}
