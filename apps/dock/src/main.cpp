#include "DockApplication.hpp"
#include <horizon/Logger.hpp>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        DockApplication app;
        LOG_INFO << "Starting Dock Overlay...";
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
