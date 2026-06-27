#include "horizon/files/FileCoverFlowView.hpp"
#include "horizon/files/FileIconProvider.hpp"
#include "horizon/files/FileListView.hpp"
#include "horizon/Application.hpp"
#include "horizon/CoverFlow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/lens/ThumbnailCache.hpp"
#include "horizon/Logger.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
#include <algorithm>
#include <cctype>
#include <xkbcommon/xkbcommon-keysyms.h>
#include "horizon/arkutils/FileOperations.hpp"

namespace horizon::files
{
    class FileCoverFlowItem : public Widget
    {
    public:
        FileCoverFlowItem() : Widget()
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(128);
            m_icon = icon.get();
            add_child(std::move(icon));
        }

        void set_data(const arkutils::FileInfo &f)
        {
            std::string thumb = lens::ThumbnailCache::get_thumbnail(f.path,
                lens::ThumbnailSize::XLarge);
            if (thumb.empty())
            {
                thumb = lens::ThumbnailCache::get_thumbnail(f.path,
                    lens::ThumbnailSize::Large);
            }
            if (thumb.empty())
            {
                thumb = lens::ThumbnailCache::get_thumbnail(f.path,
                    lens::ThumbnailSize::Normal);
            }

            if (!thumb.empty())
            {
                m_icon->set_icon_path(thumb);
            }
            else
            {
                m_icon->set_icon_name(FileIconProvider::get_icon_name(f));
            }

            // Always request XLarge if we don't have it, even if we are showing a smaller fallback
            if (lens::ThumbnailCache::get_thumbnail(f.path, lens::ThumbnailSize::XLarge).empty())
            {
                if (lens::ThumbnailCache::is_supported(f.path))
                {
                    lens::ThumbnailCache::request_thumbnail(f.path,
                        lens::ThumbnailSize::XLarge);
                }
            }
        }

    private:
        Icon *m_icon;
    };

    FileCoverFlowView::FileCoverFlowView(std::string path)
        : Widget(), m_current_path(std::move(path))
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(FILL);
        set_focusable(true);

        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        auto cover_flow = std::make_unique<horizon::CoverFlow<arkutils::FileInfo>>();
        m_cover_flow = cover_flow.get();
        m_cover_flow->set_fixed_size(300);
        m_cover_flow->set_item_factory(
            [](const arkutils::FileInfo &f, bool selected)
            {
                auto item = std::make_unique<FileCoverFlowItem>();
                item->set_data(f);
                return item;
            });

        auto navigation_label = std::make_unique<Label>("No selection");
        m_navigation_label = navigation_label.get();
        m_navigation_label->set_alignment(TextAlignment::Center);
        m_navigation_label->set_text_color(Color(1.0f, 1.0f, 1.0f));
        m_navigation_label->set_background_color(Color(0.0f, 0.0f, 0.0f));
        m_navigation_label->set_fixed_size(30);
        m_navigation_label->set_editable(true);

        m_navigation_label->when_text_edited.connect([this](const EventContext&) {
            int idx = m_cover_flow->selected_index();
            if (idx >= 0 && idx < (int)m_cover_flow->data().size()) {
                const auto &f = m_cover_flow->data()[idx];
                std::string new_name = m_navigation_label->text();
                if (new_name != FileIconProvider::get_display_name(f) && !new_name.empty()) {
                    std::filesystem::path p(f.path);
                    std::string new_path = p.parent_path() / new_name;
                    arkutils::FileOperations::rename(f.path, new_path);
                }
            }
        });

        when_key_release.connect([this](KeyEventContext &ev) {
            if (ev.keysym == XKB_KEY_F2) {
                m_navigation_label->begin_edit();
                ev.stop_propagation = true;
            }
        });

        m_cover_flow->when_selection_changed.connect(
            [this](EventContext &)
            {
                int idx = m_cover_flow->selected_index();
                if (m_list_view)
                    m_list_view->set_selected_index(idx);

                if (idx >= 0 && idx < (int)m_cover_flow->data().size())
                {
                    const auto &f = m_cover_flow->data()[idx];
                    m_navigation_label->set_text(FileIconProvider::get_display_name(f));
                }
            });

        auto list_view = std::make_unique<FileListView>(m_current_path);
        m_list_view = list_view.get();
        m_list_view->set_show_hidden_files(m_show_hidden_files);

        m_list_view->when_row_click.connect(
            [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
            {
                if (m_cover_flow)
                {
                    m_cover_flow->set_selected_index(ctx.row_index);
                    m_navigation_label->set_text(FileIconProvider::get_display_name(ctx.row_data));
                }
                when_row_click.run(ctx);
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

    FileCoverFlowView::~FileCoverFlowView()
    {
        if (m_thumbnail_timer_id != 0 && application())
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }
    }

    void FileCoverFlowView::set_show_hidden_files(bool show)
    {
        m_show_hidden_files = show;
        if (m_list_view)
        {
            m_list_view->set_show_hidden_files(show);
        }
    }

    void FileCoverFlowView::refresh(const std::string &path, const std::string &filter)
    {
        m_current_path = path;
        LOG_INFO << "Refreshing cover flow view for path: " << path << " with filter: " << filter;
        try
        {
            auto files = m_fs_model->list_directory(path);
            std::vector<arkutils::FileInfo> visible_files;

            std::string filter_lower = filter;
            std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);

            for (const auto &f : files)
            {
                if (f.is_hidden && !m_show_hidden_files) continue;

                // Apply file filter for non-directories
                if (f.type != arkutils::FileType::Directory && !m_file_filter.empty()) {
                    bool matched = false;
                    for (const auto& pat : m_file_filter) {
                        if (pat == "*" || pat == "*.*") {
                            matched = true;
                            break;
                        }
                        
                        std::string ext = pat;
                        if (ext.find("*.") == 0) ext = ext.substr(1);
                        if (f.name.length() >= ext.length() && 
                            f.name.compare(f.name.length() - ext.length(), ext.length(), ext) == 0) {
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) continue;
                }

                if (!filter.empty())
                {
                    std::string display_name = FileIconProvider::get_display_name(f);
                    std::transform(display_name.begin(), display_name.end(), display_name.begin(), ::tolower);
                    if (display_name.find(filter_lower) == std::string::npos) continue;
                }
                visible_files.push_back(f);
            }
            update_table(visible_files);
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to refresh cover flow view: " << e.what();
        }
    }

    void FileCoverFlowView::set_context_menu_factory(std::function<std::unique_ptr<horizon::Menu>(const arkutils::FileInfo &)> factory)
    {
        m_context_menu_factory = factory;
        if (m_list_view)
            m_list_view->set_context_menu_factory(factory);
    }

    void FileCoverFlowView::set_search_query(const std::string &query)
    {
        refresh(m_current_path, query);
    }

    void FileCoverFlowView::update_table(const std::vector<arkutils::FileInfo> &files)
    {
        m_cover_flow->set_data(files);
        m_list_view->update_table(files);
        start_thumbnail_watch();
    }

    std::vector<arkutils::FileInfo> FileCoverFlowView::get_selected_items() const
    {
        if (m_list_view)
            return m_list_view->get_selected_items();
        return {};
    }


    void FileCoverFlowView::start_thumbnail_watch()
    {
        if (!application() || !m_cover_flow)
            return;

        if (m_thumbnail_timer_id != 0)
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }

        m_missing_thumbnails.clear();
        for (const auto &f : m_cover_flow->data())
        {
            if (lens::ThumbnailCache::is_supported(f.path) &&
                lens::ThumbnailCache::get_thumbnail(f.path, lens::ThumbnailSize::XLarge).empty())
            {
                m_missing_thumbnails.insert(f.path);
            }
        }

        if (!m_missing_thumbnails.empty())
        {
            m_thumbnail_timer_id = application()->add_timer(3000,
                [this]()
                {
                    m_thumbnail_timer_id = 0;
                    check_thumbnails();
                });
        }
    }

    void FileCoverFlowView::check_thumbnails()
    {
        if (!application() || !m_cover_flow)
            return;

        bool has_new = false;
        for (const auto &path : m_missing_thumbnails)
        {
            if (!lens::ThumbnailCache::get_thumbnail(path, lens::ThumbnailSize::XLarge).empty())
            {
                has_new = true;
                break;
            }
        }

        if (has_new)
        {
            m_cover_flow->refresh();
        }

        start_thumbnail_watch();
    }
} // namespace horizon::files
