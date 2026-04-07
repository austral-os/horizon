#include "TopPanelApplication.hpp"
#include <horizon/Logger.hpp>
#include <exception>

int main(int argc, char *argv[])
{
    try
    {
        TopPanelApplication app;
        app.run_panel();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
