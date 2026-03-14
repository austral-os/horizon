#include "MenuManagerApplication.hpp"
#include <horizon/Logger.hpp>
#include <exception>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        MenuManagerApplication app;
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
