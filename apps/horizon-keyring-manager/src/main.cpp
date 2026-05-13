#include <horizon/Application.hpp>
#include "KeyringWindow.hpp"

using namespace horizon;
using namespace horizon::keyring;

int main(int argc, char** argv)
{
    // Application constructor: (app_id, width, height)
    Application app("horizon.keyring.manager", 900, 600);
    
    auto window = std::make_unique<KeyringWindow>(900, 600);
    app.set_root(std::move(window));
    
    app.run();
    return 0;
}
