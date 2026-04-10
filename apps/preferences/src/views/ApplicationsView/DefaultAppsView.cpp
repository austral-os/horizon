#include <views/ApplicationsView/DefaultAppsView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    DefaultAppsView::DefaultAppsView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        setup_ui();
    }

    void DefaultAppsView::calculate_layout()
    {
        // 1. Inherit width from parent if possible (for ScrollArea)
        if (m_parent) {
            m_width = m_parent->width();
        }

        // 2. Base layout calculation (distributes space to children)
        Widget::calculate_layout();

        // 3. Calculate total height for scrolling
        int total_height = m_margin * 2;
        int visible_count = 0;
        for (const auto& child : m_children) {
            if (!child->is_visible()) continue;
            
            if (child->position_type() == FREE) continue;

            if (child->fixed_size() >= 0) {
                total_height += child->fixed_size();
            }
            visible_count++;
        }
        
        if (visible_count > 1) {
            total_height += (m_spacing * (visible_count - 1));
        }

        m_height = total_height;
    }

    void DefaultAppsView::setup_ui()
    {
        // Define categories
        DefaultAppCategory internet;
        internet.name = i18n().tr("preferences.applications.categories.internet");
        internet.items.push_back({i18n().tr("preferences.applications.labels.web_browser"), "x-scheme-handler/https", {"x-scheme-handler/http", "text/html", "application/xhtml+xml"}, nullptr});
        internet.items.push_back({i18n().tr("preferences.applications.labels.email_client"), "x-scheme-handler/mailto", {}, nullptr});

        DefaultAppCategory multimedia;
        multimedia.name = i18n().tr("preferences.applications.categories.multimedia");
        multimedia.items.push_back({i18n().tr("preferences.applications.labels.image_viewer"), "image/jpeg", {"image/png", "image/gif", "image/bmp", "image/webp"}, nullptr});
        multimedia.items.push_back({i18n().tr("preferences.applications.labels.music_player"), "audio/mpeg", {"audio/mp4", "audio/ogg", "audio/flac", "audio/wav"}, nullptr});
        multimedia.items.push_back({i18n().tr("preferences.applications.labels.video_player"), "video/mp4", {"video/x-matroska", "video/webm", "video/quicktime"}, nullptr});

        DefaultAppCategory documentos;
        documentos.name = i18n().tr("preferences.applications.categories.documents");
        documentos.items.push_back({i18n().tr("preferences.applications.labels.text_editor"), "text/plain", {}, nullptr});
        documentos.items.push_back({i18n().tr("preferences.applications.labels.pdf_viewer"), "application/pdf", {}, nullptr});

        DefaultAppCategory utilidades;
        utilidades.name = i18n().tr("preferences.applications.categories.utilities");
        utilidades.items.push_back({i18n().tr("preferences.applications.labels.file_manager"), "inode/directory", {}, nullptr});
        utilidades.items.push_back({i18n().tr("preferences.applications.labels.terminal"), "x-scheme-handler/terminal", {}, nullptr});
        utilidades.items.push_back({i18n().tr("preferences.applications.labels.archiver"), "application/zip", {"application/x-tar", "application/x-compressed-tar", "application/x-7z-compressed-tar", "application/x-7z-compressed"}, nullptr});

        add_category(internet);
        add_category(multimedia);
        add_category(documentos);
        add_category(utilidades);

        // Add a spacer at the end
        add_child(horizon::Spacer());
    }

    void DefaultAppsView::add_category(const DefaultAppCategory& category)
    {
        auto cat_label = std::make_unique<Label>(category.name);
        cat_label->set_font_weight(FONT_WEIGHT_BOLD);
        cat_label->set_margin(5);
        cat_label->set_fixed_size(30);
        cat_label->set_position_type(WidgetPositionTypes::FILL);
        add_child(std::move(cat_label));

        for (const auto& item : category.items) {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(40);
            row->set_position_type(WidgetPositionTypes::FILL);
            row->set_spacing(10);
            row->set_margin(5);

            auto label = std::make_unique<Label>(item.label);
            label->set_fixed_size(250);
            label->set_position_type(WidgetPositionTypes::FILL);
            row->add_child(std::move(label));

            auto combo = std::make_unique<Combo>();
            combo->set_position_type(WidgetPositionTypes::FILL);
            auto* combo_ptr = combo.get();
            
            populate_combo(combo_ptr, item.mime_type);

            std::string mime = item.mime_type;
            std::vector<std::string> related = item.related_mimes;
            combo_ptr->when_item_selected.connect([this, mime, related](ComboItemSelectedContext& ctx) {
                on_app_selected(mime, related, ctx.item.id);
            });

            row->add_child(std::move(combo));
            add_child(std::move(row));
        }

        // Add separator spacing
        auto sep = std::make_unique<Widget>();
        sep->set_fixed_size(10);
        sep->set_position_type(WidgetPositionTypes::FILL);
        add_child(std::move(sep));
    }

    void DefaultAppsView::populate_combo(Combo* combo, const std::string& mime_type)
    {
        auto apps = DesktopManager::get_apps_for_mime(mime_type);
        if (apps.empty()) {
            combo->add_item("", i18n().tr("preferences.applications.mime_types.no_app_available"));
            return;
        }

        for (const auto& app : apps) {
            combo->add_item(app.id, app.name, app.icon);
        }

        // The first one is the default
        combo->set_selected_item_by_id(apps[0].id);
    }

    void DefaultAppsView::on_app_selected(const std::string& mime_type, const std::vector<std::string>& related_mimes, const std::string& desktop_id)
    {
        if (desktop_id.empty()) return;

        // Set default for the main MIME type
        DesktopManager::set_default_application(mime_type, desktop_id);

        // Set default for related MIME types
        for (const auto& rm : related_mimes) {
            DesktopManager::set_default_application(rm, desktop_id);
        }
    }
} // namespace horizon::preferences
