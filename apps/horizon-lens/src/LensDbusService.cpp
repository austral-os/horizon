#include "LensDbusService.hpp"
#include "LensService.hpp"
#include <horizon/Logger.hpp>

#include <dbus/dbus.h>

namespace horizon::lens
{
    LensDbusService::LensDbusService(dbusutils::DbusHelper& dbus, ThumbWorker& worker)
        : m_dbus(dbus), m_worker(worker)
    {
    }

    DBusHandlerResult LensDbusService::handle_message(DBusConnection* conn, DBusMessage* msg)
    {
        std::string interface = dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "";
        std::string method = dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "";

        if (interface == "org.horizon.Lens.Thumbnailer") {
            if (method == "RequestThumbnail") {
                handle_request_thumbnail(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            } else if (method == "ScanDirectory") {
                handle_scan_directory(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        }
        
        if (interface == "org.freedesktop.DBus.Introspectable" && method == "Introspect") {
            std::string xml = R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.horizon.Lens.Thumbnailer">
    <method name="RequestThumbnail">
      <arg name="filepath" type="s" direction="in"/>
    </method>
    <method name="ScanDirectory">
      <arg name="dirpath" type="s" direction="in"/>
    </method>
  </interface>
</node>)";
            m_dbus.send_reply(msg, {xml});
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    void LensDbusService::handle_request_thumbnail(DBusMessage* msg)
    {
        const char* filepath = nullptr;
        if (!dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &filepath, DBUS_TYPE_INVALID)) {
            LOG_ERROR << "horizon-lens: Invalid arguments for RequestThumbnail";
            return;
        }

        if (filepath) {
            LOG_INFO << "horizon-lens: D-Bus requested thumbnail for: " << filepath;
            // Enqueue it to the worker with high priority (push_front)
            m_worker.enqueue(filepath, ThumbnailSize::Large, true);
            
            // Send empty reply (void return)
            m_dbus.send_reply(msg, {});
        }
    }

    void LensDbusService::handle_scan_directory(DBusMessage* msg)
    {
        const char* dirpath = nullptr;
        if (!dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &dirpath, DBUS_TYPE_INVALID)) {
            LOG_ERROR << "horizon-lens: Invalid arguments for ScanDirectory";
            return;
        }

        if (dirpath) {
            LOG_INFO << "horizon-lens: D-Bus requested scan for directory: " << dirpath;
            if (LensService::instance() && LensService::instance()->get_scanner()) {
                LensService::instance()->get_scanner()->add_scan_path(dirpath);
            }
            
            // Send empty reply (void return)
            m_dbus.send_reply(msg, {});
        }
    }
}

