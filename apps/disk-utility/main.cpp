#include "DiskUtilityWindow.hpp"
#include <horizon/Application.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.horizon.disk-utility", 960, 700);
    app.set_name("Utilidad de Discos");

    auto window = std::make_unique<horizon::disks::DiskUtilityWindow>();
    app.set_root(std::move(window));

    app.run();
    return 0;
}
