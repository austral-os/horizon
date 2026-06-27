#include "SplashApplication.hpp"
#include <horizon/Logger.hpp>

int main(int argc, char **argv)
{
    horizon::Logger::instance().init("horizon-splash");

    try
    {
        horizon::SplashApplication app;
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "horizon-splash failed: " << e.what();
        return 1;
    }
    return 0;
}
