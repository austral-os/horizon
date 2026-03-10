#include "ArkfmView.hpp"
#include "ArkfmIconView.hpp"
#include "ArkfmListView.hpp"
#include "NavigationHistory.hpp"
#include "horizon/Application.hpp"

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
                    if (ctx.row_data.type == arkutils::FileType::Directory)
                    {
                        // We MUST defer navigation because this signal is emitted by the child
                        // we are about to destroy in navigate_to -> set_view_mode ->
                        // clear_children.
                        std::string target_path = ctx.row_data.path;
                        if (application())
                        {
                            application()->post_task([this, target_path]()
                                                     { this->navigate_to(target_path); });
                        }
                    }
                });

            add_child(std::move(view_mode_list));
        }
        else if (m_view_mode == ViewMode::Grid)
        {
            auto view_mode_grid = std::make_unique<ArkfmIconView>(m_current_path);

            view_mode_grid->when_item_dbl_click.connect(
                [this](horizon::IconViewItemMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    if (ctx.item_data.type == arkutils::FileType::Directory)
                    {
                        std::string target_path = ctx.item_data.path;
                        if (application())
                        {
                            application()->post_task([this, target_path]()
                                                     { this->navigate_to(target_path); });
                        }
                    }
                });

            add_child(std::move(view_mode_grid));
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