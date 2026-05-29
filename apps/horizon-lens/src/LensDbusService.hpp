#pragma once

#include <horizon/dbusutils/DbusHelper.hpp>
#include "ThumbWorker.hpp"
#include <string>

namespace horizon::lens
{
    class LensDbusService : public dbusutils::DbusObject
    {
    public:
        LensDbusService(dbusutils::DbusHelper& dbus, ThumbWorker& worker);
        ~LensDbusService() override = default;

        DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) override;

    private:
        dbusutils::DbusHelper& m_dbus;
        ThumbWorker& m_worker;

        void handle_request_thumbnail(DBusMessage* msg);
    };
}
