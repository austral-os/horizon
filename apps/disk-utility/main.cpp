#include "DiskUtilityWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.horizon.disk-utility", 960, 700);
    horizon::i18n().load_app_locales("disk-utility");
    app.set_name(horizon::i18n().tr("disk_utility.app_name"));

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Disk Utility");
    about.set_app_description("Manage your disks and partitions with ease.");
    about.set_app_version(APP_VERSION);
    about.set_app_icon("drive-harddisk");

    auto window = std::make_unique<horizon::disks::DiskUtilityWindow>();
    auto window_ptr = window.get();
    app.set_root(std::move(window));
    window_ptr->initialize_monitoring();

    app.run();
    return 0;
}
