#include "ViewPanel.hpp"

namespace horizon::preferences
{
    ViewPanel::ViewPanel() : GroupedIconsView()
    {
        set_alternate_colors(Color(0.98f, 0.98f, 0.98f, 1.0f), Color(1.0f, 1.0f, 1.0f, 1.0f));

        // Category: Personal
        IconGroup personal;
        personal.title = "Personal";
        personal.items = {
            {"appearance", "Apariencia", "preferences-desktop-theme"},
            {"desktop", "Escritorio", "user-desktop"},
            {"screensaver", "Salvapantallas", "preferences-desktop-screensaver"},
            {"notifications", "Notificaciones", "preferences-desktop-notification"}
        };
        add_group(personal);

        // Category: Hardware
        IconGroup hardware;
        hardware.title = "Hardware";
        hardware.items = {
            {"display", "Pantalla", "video-display"},
            {"sound", "Sonido", "preferences-desktop-sound"},
            {"mouse", "Ratón y Panel táctil", "preferences-desktop-peripherals"},
            {"keyboard", "Teclado", "preferences-desktop-keyboard"},
            {"printers", "Impresoras", "preferences-devices-printer"},
            {"power", "Energía", "preferences-system-power"}
        };
        add_group(hardware);

        // Category: Network
        IconGroup network;
        network.title = "Internet y Redes";
        network.items = {
            {"wi-fi", "Wi-Fi", "network-wireless"},
            {"bluetooth", "Bluetooth", "preferences-system-bluetooth"},
            {"network", "Red", "network-workgroup"}
        };
        add_group(network);

        // Category: System
        IconGroup system;
        system.title = "Sistema";
        system.items = {
            {"users", "Usuarios", "system-users"},
            {"datetime", "Fecha y Hora", "preferences-system-time"},
            {"region", "Región e Idioma", "preferences-desktop-locale"},
            {"applications", "Aplicaciones", "applications-other"},
            {"details", "Acerca de", "help-about"}
        };
        add_group(system);
    }
} // namespace horizon::preferences
