#include "SystemObserver.hpp"
#include <iostream>

namespace horizon::notifications
{
    SystemObserver::SystemObserver(NotificationManager &manager)
        : m_manager(manager)
    {
    }

    SystemObserver::~SystemObserver()
    {
        stop();
    }

    void SystemObserver::start()
    {
        m_running = true;
        m_thread = std::thread(&SystemObserver::run, this);
    }

    void SystemObserver::stop()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void SystemObserver::run()
    {
        try
        {
            // We use the System Bus for UPower and NetworkManager
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
            
            // Listen for UPower (Power/Battery)
            m_dbus->add_match_rule("type='signal',sender='org.freedesktop.UPower',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'");
            
            // Listen for NetworkManager
            m_dbus->add_match_rule("type='signal',sender='org.freedesktop.NetworkManager',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'");

            while (m_running)
            {
                DBusMessage *msg = m_dbus->pop_message(500);
                if (msg)
                {
                    handle_message(msg);
                    dbus_message_unref(msg);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "SystemObserver Error: " << e.what() << std::endl;
        }
    }

    void SystemObserver::handle_message(DBusMessage *msg)
    {
        const char *sender = dbus_message_get_sender(msg);
        if (!sender) return;

        std::string s(sender);
        if (s.find("org.freedesktop.UPower") != std::string::npos)
        {
            handle_upower_event(msg);
        }
        else if (s.find("org.freedesktop.NetworkManager") != std::string::npos)
        {
            handle_nm_event(msg);
        }
    }

    void SystemObserver::handle_upower_event(DBusMessage *msg)
    {
        // Simple check for "OnBattery" property change
        // For brevity in this implementation, we just trigger a generic notification
        // in a real scenario we would parse the changed properties dict.
        
        // Let's at least check if it's the main UPower object
        if (std::string(dbus_message_get_path(msg)) == "/org/freedesktop/UPower")
        {
            // We could fetch the current state
            auto variant = m_dbus->get_property("org.freedesktop.UPower", "/org/freedesktop/UPower", 
                                              "org.freedesktop.UPower", "OnBattery");
            
            if (std::holds_alternative<bool>(variant))
            {
                bool on_battery = std::get<bool>(variant);
                if (on_battery != m_last_on_battery)
                {
                    m_last_on_battery = on_battery;
                    if (on_battery)
                        m_manager.add_notification("System", "battery-caution", "Energía", "Cargador desconectado (Usando batería)", 3000);
                    else
                        m_manager.add_notification("System", "battery-charging", "Energía", "Cargador conectado (Cargando)", 3000);
                }
            }
        }
    }

    void SystemObserver::handle_nm_event(DBusMessage *msg)
    {
        // Similar logic for NetworkManager
        // For now, just a placeholder that detects "something changed" in NM
        // In a real app, we'd check Connectivity property.
    }
}
