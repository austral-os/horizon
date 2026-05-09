#include "NotificationApplication.hpp"
#include <horizon/Logger.hpp>
#include <exception>

using namespace horizon::notifications;

int main(int argc, char *argv[])
{
    try
    {
        NotificationApplication app;
        app.run_notifications();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Notification App Error: " << e.what();
        return 1;
    }

    return 0;
}
