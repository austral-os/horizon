#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include "secrets/Service.hpp"

using namespace horizon;
using namespace horizon::dbusutils;

int main(int argc, char** argv)
{
    Logger::instance().init("horizon-keyring");
    std::cout << "[Horizon Keyring] Starting Secret Service daemon..." << std::endl;

    try {
        DbusHelper dbus(DBUS_BUS_SESSION);

        bool secrets_ok = dbus.request_name("org.freedesktop.Secrets");
        bool secrets_lc_ok = dbus.request_name("org.freedesktop.secrets");
        bool gnome_ok = dbus.request_name("org.gnome.keyring");

        if (!secrets_ok && !secrets_lc_ok && !gnome_ok) {
            std::cerr << "[Horizon Keyring] Failed to request D-Bus names. Is another keyring running?" << std::endl;
            return 1;
        }

        std::cout << "[Horizon Keyring] Names acquired: " 
                  << (secrets_ok ? "org.freedesktop.Secrets " : "")
                  << (secrets_lc_ok ? "org.freedesktop.secrets " : "")
                  << (gnome_ok ? "org.gnome.keyring" : "") << std::endl;

        // Initialize the main service object
        auto service = std::make_unique<horizon::secrets::Service>(dbus);
        dbus.register_fallback("/", service.get());

        std::cout << "[Horizon Keyring] Service registered at / (Root fallback)" << std::endl;

        // Event loop
        int iterations = 0;
        while (dbus.process_events(100)) {
            iterations++;
            if (iterations % 50 == 0) { // Every ~5 seconds
                printf("[Horizon Keyring] Heartbeat: Service is alive...\n");
                fflush(stdout);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Horizon Keyring] Critical error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
