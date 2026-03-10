#include "ArkfmApplication.hpp"
#include "ArkfmWindow.hpp"

namespace horizon::arkfm
{

    const int ARK_APP_DEFAULT_WIDTH = 1000;
    const int ARK_APP_DEFAULT_HEIGHT = 800;

    ArkfmApplication::ArkfmApplication()
        : Application("org.horizon.arkfm", ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT)
    {
        auto window = std::make_unique<ArkfmWindow>(ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT);
        set_root(std::move(window));
    }

} // namespace horizon::arkfm