#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <memory>
#include "PortalService.hpp"

using namespace horizon;
using namespace horizon::dbusutils;

int main(int argc, char** argv)
{
    Logger::instance().init("xdg-desktop-portal-horizon");
    std::cout << "[Horizon Portal] Starting Desktop Portal Backend..." << std::endl;

    try {
        DbusHelper dbus(DBUS_BUS_SESSION);

        bool name_ok = dbus.request_name("org.freedesktop.impl.portal.desktop.horizon");

        if (!name_ok) {
            std::cerr << "[Horizon Portal] Failed to request D-Bus name. Is another instance running?" << std::endl;
            return 1;
        }

        std::cout << "[Horizon Portal] Name acquired: org.freedesktop.impl.portal.desktop.horizon" << std::endl;

        // Initialize the main service object
        auto service = std::make_unique<horizon::portal::PortalService>(dbus);
        dbus.register_object("/org/freedesktop/portal/desktop", service.get());

        std::cout << "[Horizon Portal] Service registered at /org/freedesktop/portal/desktop" << std::endl;

        // Event loop
        while (dbus.process_events(100)) {
            // Check and execute queued watcher tasks on main thread
            service->process_tasks();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Horizon Portal] Critical error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
