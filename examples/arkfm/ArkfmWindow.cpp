#include "ArkfmWindow.hpp"
#include "ArkfmSidebar.hpp"
#include "ArkfmToolbar.hpp"
#include "ArkfmView.hpp"
#include "horizon/ApplicationWindow.hpp"
#include "horizon/VPanel.hpp"

namespace horizon::arkfm
{

    ArkfmWindow::ArkfmWindow(int w, int h) : ApplicationWindow("Ark File Manager")
    {
        set_size(w, h);
        toolbar()->add_toolbar_widget(std::make_unique<ArkToolbar>());
        show_status_bar();

        auto vpanel = std::make_unique<horizon::VPanel>();
        vpanel->set_spacing(10);

        auto sidebar = std::make_unique<ArkfmSidebar>();
        auto view = std::make_unique<ArkfmView>();

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(view));

        set_content(std::move(vpanel));
    }

} // namespace horizon::arkfm