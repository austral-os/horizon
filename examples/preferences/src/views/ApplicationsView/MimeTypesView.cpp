#include <views/ApplicationsView/MimeTypesView.hpp>
#include <horizon/TreeViewItem.hpp>
#include <filesystem>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

namespace horizon::preferences
{
    MimeTypesView::MimeTypesView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);

        // Sidebar TreeView
        auto tree_view = std::make_unique<TreeView>();
        tree_view->set_width(250);
        m_tree_view = tree_view.get();
        add_child(std::move(tree_view));

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

        // Connect signal
        m_tree_view->when_item_selected.connect([this](TreeViewItem* item) {
            if (item) {
                // If it's a leaf node (not a category)
                if (!item->has_children()) {
                    m_selected_mime_label->set_text("MIME Type seleccionado: " + item->get_text());
                }
            }
        });

        load_mime_types();
    }

    void MimeTypesView::load_mime_types()
    {
        if (!m_tree_view) return;

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
                
                // Skip special directories
                if (std::find(skip_dirs.begin(), skip_dirs.end(), category) != skip_dirs.end()) {
                    continue;
                }

                auto category_item = std::make_unique<TreeViewItem>("folder", category);
                category_item->set_bold(true);

                bool has_mimes = false;
                for (const auto& mime_entry : fs::directory_iterator(entry.path())) {
                    if (mime_entry.is_regular_file() && mime_entry.path().extension() == ".xml") {
                        std::string mime_name = mime_entry.path().stem().string();
                        // Special case: skip files starting with 'x-' if you want, or just add them.
                        category_item->add_item(std::make_unique<TreeViewItem>("text-x-generic", mime_name));
                        has_mimes = true;
                    }
                }

                if (has_mimes) {
                    m_tree_view->add_root_item(std::move(category_item));
                }
            }
        }
    }
} // namespace horizon::preferences
