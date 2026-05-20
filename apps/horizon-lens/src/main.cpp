#include "LensService.hpp"
#include <horizon/Logger.hpp>

int main(int argc, char** argv)
{
    horizon::Logger::instance().init("horizon-lens");

    LOG_INFO << "horizon-lens: Initializing background thumbnail service...";

    horizon::lens::LensService service;
    service.run();

    return 0;
}
