#include "ArkfmView.hpp"
#include "ArkfmCoverFlowView.hpp"
#include "ArkfmIconView.hpp"
#include "ArkfmListView.hpp"
#include "ArkfmWindow.hpp"
#include "NavigationHistory.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <sys/stat.h>

namespace horizon::arkfm
{

    ArkfmView::ArkfmView(std::string path)
        : Widget(), m_current_path(std::move(path)),
          m_history(std::make_unique<NavigationHistory>())
    {
        m_history->push(m_current_path);
        set_view_mode(ViewMode::List);
    }

    ArkfmView::~ArkfmView() = default;

    void ArkfmView::set_view_mode(ViewMode vm)
    {
        m_view_mode = vm;
        clear_children();

        if (m_view_mode == ViewMode::List)
        {
            auto view_mode_list = std::make_unique<ArkfmListView>(m_current_path);

            view_mode_list->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });

            add_child(std::move(view_mode_list));
        }
        else if (m_view_mode == ViewMode::Grid)
        {
            auto view_mode_grid = std::make_unique<ArkfmIconView>(m_current_path);

            view_mode_grid->when_item_dbl_click.connect(
                [this](horizon::IconViewItemMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.item_data);
                });

            add_child(std::move(view_mode_grid));
        }
        else if (m_view_mode == ViewMode::CoverFlow)
        {
            auto view_mode_cover = std::make_unique<ArkfmCoverFlowView>(m_current_path);

            // Handle navigation via double click if desired, but user didn't ask yet.
            // For now, just show it.

            view_mode_cover->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });

            add_child(std::move(view_mode_cover));
        }

        // Apply existing search query if any
        if (!m_search_query.empty())
        {
            if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
            else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
            else if (auto *child = dynamic_cast<ArkfmCoverFlowView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
        }
    }

    void ArkfmView::navigate_to(const std::string &path, bool record_history)
    {
        m_current_path = path;
        if (record_history)
        {
            m_history->push(m_current_path);
        }

        set_view_mode(m_view_mode);

        PathChangedEvent event;
        event.path = m_current_path;
        when_path_changed.run(event);
    }

    void ArkfmView::navigate_back()
    {
        if (m_history->can_back())
        {
            navigate_to(m_history->back(), false);
        }
    }

    void ArkfmView::navigate_forward()
    {
        if (m_history->can_forward())
        {
            navigate_to(m_history->forward(), false);
        }
    }

    std::vector<arkutils::FileInfo> ArkfmView::get_selection() const
    {
        if (m_children.empty())
            return {};

        if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
            return child->get_selected_items();
        else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
            return child->get_selected_items();
        // TODO: CoverFlowView if it supports selection

        return {};
    }

    void ArkfmView::open_selection()
    {
        auto sel = get_selection();
        if (sel.empty())
            return;

        open_item(sel[0]);
    }

    void ArkfmView::open_item(const arkutils::FileInfo &f)
    {
        if (f.type == arkutils::FileType::Directory)
        {
            // We MUST defer navigation because this signal is emitted by the child
            // we are about to destroy in navigate_to -> set_view_mode ->
            // clear_children.
            std::string target_path = f.path;
            if (application())
            {
                application()->post_task([this, target_path]()
                                         { this->navigate_to(target_path); });
            }
        }
        else if (f.extension == "desktop")
        {
            ApplicationLauncher::launch_from_desktop_file(f.path);
        }
        else if (f.permissions & (S_IXUSR | S_IXGRP | S_IXOTH))
        {
            if (auto *win = dynamic_cast<ArkfmWindow *>(application()->root()))
            {
                if (win->confirm("¿Desea ejecutar esta aplicación?", "Confirmar ejecución"))
                {
                    ApplicationLauncher::launch_binary(f.path);
                }
            }
        }
        else
        {
            // Generic file opening via xdg-mime
            ApplicationLauncher::open_file(f.path);
        }
    }

    void ArkfmView::set_search_query(const std::string &query)
    {
        m_search_query = query;
        if (m_children.empty())
            return;

        // Refresh the active view with the new filter
        if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<ArkfmCoverFlowView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
    }

    bool ArkfmView::can_back() const
    {
        return m_history->can_back();
    }

    bool ArkfmView::can_forward() const
    {
        return m_history->can_forward();
    }

    const std::string &ArkfmView::current_path() const
    {
        return m_current_path;
    }

} // namespace horizon::arkfm