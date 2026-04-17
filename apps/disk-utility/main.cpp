#include "DiskUtilityWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.horizon.disk-utility", 960, 700);
    horizon::i18n().load_app_locales("disk-utility");
    app.set_name(horizon::i18n().tr("disk_utility.app_name"));

    auto window = std::make_unique<horizon::disks::DiskUtilityWindow>();
    auto window_ptr = window.get();
    app.set_root(std::move(window));
    window_ptr->initialize_monitoring();

    app.run();
    return 0;
}
