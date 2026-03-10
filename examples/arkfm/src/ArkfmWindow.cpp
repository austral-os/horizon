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
        auto ark_toolbar = std::make_unique<ArkToolbar>();
        auto *ark_toolbar_ptr = ark_toolbar.get();
        toolbar()->add_toolbar_widget(std::move(ark_toolbar));
        show_status_bar();

        auto vpanel = std::make_unique<horizon::VPanel>();
        vpanel->set_spacing(10);

        auto sidebar = std::make_unique<ArkfmSidebar>();
        auto view = std::make_unique<ArkfmView>(getenv("HOME") ? getenv("HOME") : "~/");
        auto *view_ptr = view.get();

        ark_toolbar_ptr->when_navigation_clicked.connect(
            [view_ptr](NavigationButtonClickEvent &ctx)
            {
                if (ctx.index == 0)
                {
                    view_ptr->navigate_back();
                }
                else
                {
                    view_ptr->navigate_forward();
                }
            });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(view));

        set_content(std::move(vpanel));
    }

} // namespace horizon::arkfm