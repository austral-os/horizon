#include "ArkfmWindow.hpp"
#include "ArkfmSidebar.hpp"
#include "ArkfmToolbar.hpp"
#include "ArkfmView.hpp"
#include "dialogs/NewFolderDialog.hpp"
#include "horizon/ApplicationWindow.hpp"
#include "horizon/VPanel.hpp"
#include "horizon/arkutils/FileOperations.hpp"

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

        ark_toolbar_ptr->when_view_mode_changed.connect(
            [view_ptr](ViewModeChangeEvent &ctx)
            {
                if (ctx.view_mode_index == 0)
                {
                    view_ptr->set_view_mode(ViewMode::Grid);
                }
                else if (ctx.view_mode_index == 1)
                {
                    view_ptr->set_view_mode(ViewMode::List);
                }
                else if (ctx.view_mode_index == 3)
                {
                    view_ptr->set_view_mode(ViewMode::CoverFlow);
                }
            });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(view));
        set_content(std::move(vpanel));

        this->when_application_load.connect(
            [this, view_ptr](EventContext &)
            {
                if (application())
                {
                    application()->signal_manager.connect(
                        "new-folder",
                        [this, view_ptr](SignalContext &)
                        {
                            auto dialog = std::make_unique<NewFolderDialog>();
                            dialog->when_accepted.connect(
                                [this, view_ptr](NewFolderEvent &ctx)
                                {
                                    std::string full_path =
                                        view_ptr->current_path() + "/" + ctx.folder_name;
                                    arkutils::FileOperations::create_directory(full_path);
                                    view_ptr->navigate_to(view_ptr->current_path());
                                });
                            dialog->run();
                        });
                }
            });
    }

} // namespace horizon::arkfm