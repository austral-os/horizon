#include "WallApplication.hpp"
#include <horizon/Logger.hpp>
#include <exception>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        std::string wall_path = (argc > 1) ? argv[1] : "";
        WallApplication app(wall_path);
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
