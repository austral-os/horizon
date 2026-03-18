#include "ArkfmCoverFlowView.hpp"
#include "ArkfmFileProvider.hpp"
#include "ArkfmListView.hpp"
#include "horizon/Application.hpp"
#include "horizon/CoverFlow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"

namespace horizon::arkfm
{
    struct PathChangedEvent : public EventContext
    {
        std::string path;
    };

    class ArkfmCoverFlowItem : public Widget
    {
    public:
        ArkfmCoverFlowItem() : Widget()
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(128);
            icon->set_enabled(false);
            m_icon = icon.get();

            add_child(std::move(icon));
        }

        void set_data(const arkutils::FileInfo &f)
        {
            m_icon->set_icon_name(ArkfmFileProvider::get_icon_name(f));
        }

    private:
        Icon *m_icon;
    };

    ArkfmCoverFlowView::ArkfmCoverFlowView(std::string path)
        : Widget(), m_current_path(std::move(path))
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(FILL);

        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        auto cover_flow = std::make_unique<horizon::CoverFlow<arkutils::FileInfo>>();
        m_cover_flow = cover_flow.get();
        m_cover_flow->set_fixed_size(300);
        m_cover_flow->set_item_factory(
            [](const arkutils::FileInfo &f, bool selected)
            {
                auto item = std::make_unique<ArkfmCoverFlowItem>();
                item->set_data(f);
                return item;
            });

        // Navigation Label
        auto navigation_label = std::make_unique<Label>("No selection");
        m_navigation_label = navigation_label.get();
        m_navigation_label->set_alignment(TextAlignment::Center);
        m_navigation_label->set_text_color(Color(1.0f, 1.0f, 1.0f));
        m_navigation_label->set_background_color(Color(0.0f, 0.0f, 0.0f)); // Black background
        m_navigation_label->set_fixed_size(30);

        // Communication
        m_cover_flow->when_selection_changed.connect(
            [this](EventContext &)
            {
                int idx = m_cover_flow->selected_index();
                if (m_list_view)
                {
                    m_list_view->set_selected_index(idx);
                }

                if (idx >= 0 && idx < (int)m_cover_flow->data().size())
                {
                    const auto &f = m_cover_flow->data()[idx];
                    m_navigation_label->set_text(ArkfmFileProvider::get_display_name(f));
                }
            });

        auto list_view = std::make_unique<ArkfmListView>(m_current_path);
        m_list_view = list_view.get();

        m_list_view->when_row_click.connect(
            [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
            {
                if (m_cover_flow)
                {
                    m_cover_flow->set_selected_index(ctx.row_index);
                    m_navigation_label->set_text(ArkfmFileProvider::get_display_name(ctx.row_data));
                }
            });

        m_list_view->when_row_dbl_click.connect(
            [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
            {
                if (ctx.row_data.type == arkutils::FileType::Directory)
                {
                    when_row_dbl_click.run(ctx);
                }
            });

        add_child(std::move(cover_flow));
        add_child(std::move(navigation_label));
        add_child(std::move(list_view));

        refresh(m_current_path);
    }

    ArkfmCoverFlowView::~ArkfmCoverFlowView() = default;

    void ArkfmCoverFlowView::refresh(const std::string &path)
    {
        m_current_path = path;
        try
        {
            auto files = m_fs_model->list_directory(path);
            std::vector<arkutils::FileInfo> visible_files;
            for (const auto &f : files)
            {
                if (!f.is_hidden)
                {
                    visible_files.push_back(f);
                }
            }
            update_data(visible_files);
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to refresh cover flow view: " << e.what();
        }
    }

    void ArkfmCoverFlowView::update_data(const std::vector<arkutils::FileInfo> &files)
    {
        m_cover_flow->set_data(files);
        m_list_view->update_table(files);
    }

} // namespace horizon::arkfm
