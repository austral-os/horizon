#include "ViewPanel.hpp"
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    ViewPanel::ViewPanel() : GroupedIconsView()
    {
        set_alternate_colors(Color(0.98f, 0.98f, 0.98f, 1.0f), Color(1.0f, 1.0f, 1.0f, 1.0f));

        // Category: Personal
        IconGroup personal;
        personal.title = i18n().tr("preferences.groups.personal");
        personal.items = {
            {"appearance", i18n().tr("preferences.sections.appearance"), "preferences-desktop-theme"},
            {"desktop", i18n().tr("preferences.sections.desktop"), "user-desktop"},
            {"screensaver", i18n().tr("preferences.sections.screensaver"), "preferences-desktop-screensaver"},
            {"notifications", i18n().tr("preferences.sections.notifications"), "preferences-desktop-notification"}
        };
        add_group(personal);

        // Category: Hardware
        IconGroup hardware;
        hardware.title = i18n().tr("preferences.groups.hardware");
        hardware.items = {
            {"display", i18n().tr("preferences.sections.display"), "video-display"},
            {"sound", i18n().tr("preferences.sections.sound"), "preferences-desktop-sound"},
            {"mouse", i18n().tr("preferences.sections.mouse"), "preferences-desktop-peripherals"},
            {"keyboard", i18n().tr("preferences.sections.keyboard"), "preferences-desktop-keyboard"},
            {"printers", i18n().tr("preferences.sections.printers"), "preferences-devices-printer"},
            {"power", i18n().tr("preferences.sections.power"), "preferences-system-power"}
        };
        add_group(hardware);

        // Category: Network
        IconGroup network;
        network.title = i18n().tr("preferences.groups.network");
        network.items = {
            {"wi-fi", i18n().tr("preferences.sections.wifi"), "network-wireless"},
            {"bluetooth", i18n().tr("preferences.sections.bluetooth"), "preferences-system-bluetooth"},
            {"network", i18n().tr("preferences.sections.network"), "network-workgroup"}
        };
        add_group(network);

        // Category: System
        IconGroup system;
        system.title = i18n().tr("preferences.groups.system");
        system.items = {
            {"users", i18n().tr("preferences.sections.users"), "system-users"},
            {"datetime", i18n().tr("preferences.sections.datetime"), "preferences-system-time"},
            {"region", i18n().tr("preferences.sections.region"), "preferences-desktop-locale"},
            {"applications", i18n().tr("preferences.sections.applications"), "applications-other"},
            {"details", i18n().tr("preferences.sections.details"), "help-about"}
        };
        add_group(system);
    }
} // namespace horizon::preferences
