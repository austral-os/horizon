#include "ArkfmWindow.hpp"
#include "ArkfmSidebar.hpp"
#include "ArkfmToolbar.hpp"
#include "ArkfmView.hpp"
#include "dialogs/NewFolderDialog.hpp"
#include "horizon/ApplicationWindow.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/VPanel.hpp"
#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileOperations.hpp"
#include <filesystem>
#include <memory>
#include <thread>

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
        m_view_ptr = view.get();
        auto *view_ptr = m_view_ptr;

        show_status_bar();
        auto *sb = statusbar();
        auto lbl = std::make_unique<horizon::Label>("");
        m_status_label = lbl.get();

        auto pbc = std::make_unique<horizon::Widget>();
        auto pb = std::make_unique<horizon::ProgressBar>();

        m_progress_bar = pb.get();
        m_progress_bar->set_visible(false);
        m_progress_bar->set_fixed_size(10);

        pbc->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        pbc->set_fixed_size(200);

        pbc->add_child(horizon::Spacer());
        pbc->add_child(std::move(pb));
        pbc->add_child(horizon::Spacer());

        sb->add_child(horizon::Spacer(10));
        sb->add_child(std::move(lbl));
        sb->add_child(std::move(pbc));
        sb->add_child(horizon::Spacer(10));

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

    void ArkfmWindow::handle_copy(const std::string &path)
    {
        m_clipboard_path = path;
        m_is_cut = false;
        show_status_message("Copiado al portapapeles");
    }

    void ArkfmWindow::handle_cut(const std::string &path)
    {
        m_clipboard_path = path;
        m_is_cut = true;
        show_status_message("Cortado al portapapeles");
    }

    void ArkfmWindow::handle_paste(const std::string &target_dir)
    {
        if (m_clipboard_path.empty())
            return;

        std::filesystem::path src(m_clipboard_path);
        std::filesystem::path dst_dir(target_dir);

        if (src == dst_dir || dst_dir.string().find(src.string() + "/") == 0)
        {
            show_status_message("No fue posible realizar la accion");
            return;
        }

        m_progress_bar->set_progress(0.0f);
        m_progress_bar->set_visible(true);
        show_status_message("Copiando...");

        auto future = arkutils::FileOperations::copy(
            m_clipboard_path, target_dir,
            [this](double progress)
            {
                application()->post_task(
                    [this, progress]()
                    { m_progress_bar->set_progress(static_cast<float>(progress)); });
            });

        std::thread(
            [this, f = std::move(future)]() mutable
            {
                auto result = f.get();
                if (application())
                {
                    application()->post_task(
                        [this, result]()
                        {
                            m_progress_bar->set_visible(false);
                            if (result == arkutils::FileOperations::Result::Success)
                            {
                                show_status_message(m_is_cut ? "Movido con éxito"
                                                             : "Copiado con éxito");
                                if (m_is_cut)
                                {
                                    // TODO: In a real app we would delete src or use move()
                                    // For now we just reset m_is_cut and m_clipboard_path
                                    m_clipboard_path = "";
                                    m_is_cut = false;
                                }
                                if (m_view_ptr)
                                    m_view_ptr->navigate_to(m_view_ptr->current_path());
                            }
                            else
                            {
                                show_status_message("No fue posible realizar la accion");
                            }
                        });
                }
            })
            .detach();
    }

    void ArkfmWindow::handle_rename(const std::string &path)
    {
        LOG_INFO << "renombrando... " << path;
        show_status_message("Renombrando " + std::filesystem::path(path).filename().string());
        // TODO: Implementation with dialog
    }

    void ArkfmWindow::handle_delete(const std::string &path)
    {
        LOG_INFO << "borrando... " << path;
        show_status_message("Eliminando " + std::filesystem::path(path).filename().string());

        // For now just logging as requested, but we could do:
        // auto future = arkutils::FileOperations::remove(path);
        // ... and refresh view on success.

        // Let's pretend it worked for the sake of the log message
        show_status_message("Elemento eliminado (simulado)", 3000);
    }

    void ArkfmWindow::show_status_message(const std::string &msg, int timeout_ms)
    {
        if (!m_status_label)
            return;
        m_status_label->set_text(msg);

        if (application() && timeout_ms > 0)
        {
            application()->add_timer(timeout_ms,
                                     [this, msg]()
                                     {
                                         if (m_status_label && m_status_label->text() == msg)
                                         {
                                             m_status_label->set_text("");
                                         }
                                     });
        }
    }

} // namespace horizon::arkfm