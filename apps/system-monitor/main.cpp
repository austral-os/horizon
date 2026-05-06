#include "SystemMonitorWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>("org.horizon.system-monitor", 1000, 700);

    // Load translations
    horizon::i18n().load_app_locales("system-monitor");

    app->set_name(horizon::i18n().tr("system_monitor.title"));
    app->set_icon_name("utilities-system-monitor");
    
    // Setup About info
    auto &about = app->about_manager();
    about.set_app_title(horizon::i18n().tr("system_monitor.title"));
    about.set_app_description(horizon::i18n().tr("system_monitor.about"));
    about.set_app_version("0.1.0");
    about.set_app_icon("utilities-system-monitor");
    about.add_app_translator("Horacio", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com");

    auto window = std::make_unique<SystemMonitorWindow>();

    app->set_root(std::move(window));

    app->run();
    return 0;
}
