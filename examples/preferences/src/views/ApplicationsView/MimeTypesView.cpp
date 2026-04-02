#include <views/ApplicationsView/MimeTypesView.hpp>
#include <horizon/TreeViewItem.hpp>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace horizon::preferences
{
    MimeTypesView::MimeTypesView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);

        // Sidebar Left Column
        auto left_column = std::make_unique<Widget>();
        left_column->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_column->set_width(250);
        left_column->set_margin(10);
        left_column->set_spacing(10);

        auto search_box = std::make_unique<SearchBox>();
        search_box->set_placeholder("Buscar MIME types...");
        m_search_box = search_box.get();
        left_column->add_child(std::move(search_box));

        auto tree_view = std::make_unique<TreeView>();
        tree_view->set_position_type(WidgetPositionTypes::FILL);
        m_tree_view = tree_view.get();
        left_column->add_child(std::move(tree_view));

        add_child(std::move(left_column));

        // Content Area
        auto content_area = std::make_unique<Widget>();
        content_area->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content_area->set_position_type(WidgetPositionTypes::FILL);
        content_area->set_margin(20);

        auto selected_label = std::make_unique<Label>("Seleccione un MIME type de la lista");
        selected_label->set_position_type(WidgetPositionTypes::FILL);
        m_selected_mime_label = selected_label.get();
        content_area->add_child(std::move(selected_label));

        add_child(std::move(content_area));

        // Signals
        m_tree_view->when_item_selected.connect([this](TreeViewItem* item) {
            if (item && !item->has_children()) {
                m_selected_mime_label->set_text("MIME Type seleccionado: " + item->get_text());
            }
        });

        m_search_box->when_text_changed.connect([this](KeyEventContext&) {
            update_tree(m_search_box->text());
        });

        load_mime_types();
    }

    void MimeTypesView::load_mime_types()
    {
        m_mime_data.clear();
        std::string mime_path = "/usr/share/mime/";
        if (!fs::exists(mime_path)) return;

        std::vector<std::string> skip_dirs = {
            "packages", "aliases", "subclasses", "generic-icons", 
            "icons", "treemagic", "types", "XMLnamespaces", 
            "globs", "globs2", "magic", "version", "mime.cache"
        };

        for (const auto& entry : fs::directory_iterator(mime_path)) {
            if (entry.is_directory()) {
                std::string category = entry.path().filename().string();
                if (std::find(skip_dirs.begin(), skip_dirs.end(), category) != skip_dirs.end()) continue;

                std::vector<std::string> mimes;
                for (const auto& mime_entry : fs::directory_iterator(entry.path())) {
                    if (mime_entry.is_regular_file() && mime_entry.path().extension() == ".xml") {
                        mimes.push_back(mime_entry.path().stem().string());
                    }
                }
                
                if (!mimes.empty()) {
                    std::sort(mimes.begin(), mimes.end());
                    m_mime_data[category] = mimes;
                }
            }
        }
        update_tree();
    }

    void MimeTypesView::update_tree(const std::string& filter)
    {
        if (!m_tree_view) return;
        m_tree_view->clear_root_items();

        std::string query = filter;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        for (const auto& [category, mimes] : m_mime_data) {
            std::vector<std::string> filtered_mimes;
            
            for (const auto& mime : mimes) {
                std::string mime_lower = mime;
                std::transform(mime_lower.begin(), mime_lower.end(), mime_lower.begin(), ::tolower);
                
                if (query.empty() || mime_lower.find(query) != std::string::npos) {
                    filtered_mimes.push_back(mime);
                }
            }

            if (!filtered_mimes.empty()) {
                auto category_item = std::make_unique<TreeViewItem>("folder", category);
                category_item->set_bold(true);
                if (!query.empty()) category_item->set_expanded(true);

                for (const auto& mime : filtered_mimes) {
                    category_item->add_item(std::make_unique<TreeViewItem>("text-x-generic", mime));
                }
                m_tree_view->add_root_item(std::move(category_item));
            }
        }
    }
} // namespace horizon::preferences
