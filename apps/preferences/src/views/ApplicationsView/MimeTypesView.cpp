#include <views/ApplicationsView/MimeTypesView.hpp>
#include <views/ApplicationsView/InputDialog.hpp>
#include <utils/DesktopManager.hpp>
#include <horizon/TreeViewItem.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/MessageDialog.hpp>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <fstream>
#include <views/ApplicationsView/AppPickerDialog.hpp>
#include <horizon/I18n.hpp>
#include <regex>

namespace fs = std::filesystem;

namespace horizon::preferences
{
    MimeTypesView::MimeTypesView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);

        setup_left_column();
        setup_right_column();

        load_mime_types();
    }

    void MimeTypesView::setup_left_column()
    {
        auto left_column = std::make_unique<Widget>();
        left_column->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_column->set_width(250);
        left_column->set_margin(10);
        left_column->set_spacing(10);

        auto search_box = std::make_unique<SearchBox>();
        search_box->set_placeholder(i18n().tr("preferences.applications.mime_types.search_placeholder"));
        m_search_box = search_box.get();
        left_column->add_child(std::move(search_box));

        auto tree_view = std::make_unique<TreeView>();
        tree_view->set_position_type(WidgetPositionTypes::FILL);
        m_tree_view = tree_view.get();
        left_column->add_child(std::move(tree_view));

        add_child(std::move(left_column));

        // Tree selection signal
        m_tree_view->when_item_selected.connect([this](TreeViewItem* item) {
            if (item && !item->has_children()) {
                // Find category by looking at parent
                std::string category = "";
                if (item->parent()) {
                    if (auto* parent_item = dynamic_cast<TreeViewItem*>(item->parent())) {
                        category = parent_item->get_text();
                    }
                }
                m_current_mime = category + "/" + item->get_text();
                update_details(m_current_mime);
            } else {
                m_current_mime = "";
                update_details("");
            }
        });

        // Search signal
        m_search_box->when_text_changed.connect([this](KeyEventContext&) {
            update_tree(m_search_box->text());
        });
    }

    void MimeTypesView::setup_right_column()
    {
        auto right_column = std::make_unique<Widget>();
        right_column->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_column->set_position_type(WidgetPositionTypes::FILL);
        right_column->set_margin(10);
        right_column->set_spacing(15);

        auto title = std::make_unique<Label>(i18n().tr("preferences.applications.mime_types.select_mime"));
        title->set_fixed_size(30);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        m_mime_title_label = title.get();
        right_column->add_child(std::move(title));

        // --- Extensiones Section ---
        auto ext_label = std::make_unique<Label>(i18n().tr("preferences.applications.mime_types.associated_extensions"));
        ext_label->set_fixed_size(25);
        right_column->add_child(std::move(ext_label));

        auto ext_table = std::make_unique<TableView<MimeExtension>>();
        m_extensions_table = ext_table.get();
        m_extensions_table->set_header_visible(false);
        
        TableColumn<MimeExtension> col_ext;
        col_ext.width = 300;
        col_ext.cell_factory = [](const MimeExtension& ext) {
            return std::make_unique<Label>(ext.pattern);
        };
        m_extensions_table->add_column(col_ext);
        right_column->add_child(std::move(ext_table));

        auto ext_toolbar = std::make_unique<Widget>();
        ext_toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        ext_toolbar->set_fixed_size(32);
        ext_toolbar->set_spacing(5);

        auto btn_add_ext = std::make_unique<Button<AquaObject>>();
        btn_add_ext->set_fixed_size(32);
        auto icon_add = std::make_unique<Icon>();
        icon_add->set_icon_name("list-add");
        icon_add->set_icon_size(16);
        btn_add_ext->add_child(std::move(icon_add));
        btn_add_ext->when_click.connect([this](MouseButtonEventContext&){ on_add_extension(); });
        ext_toolbar->add_child(std::move(btn_add_ext));

        auto btn_rem_ext = std::make_unique<Button<AquaObject>>();
        btn_rem_ext->set_fixed_size(32);
        auto icon_rem = std::make_unique<Icon>();
        icon_rem->set_icon_name("list-remove");
        icon_rem->set_icon_size(16);
        btn_rem_ext->add_child(std::move(icon_rem));
        btn_rem_ext->when_click.connect([this](MouseButtonEventContext&){ on_remove_extension(); });
        ext_toolbar->add_child(std::move(btn_rem_ext));

        right_column->add_child(std::move(ext_toolbar));

        // --- Aplicaciones Section ---
        auto app_label = std::make_unique<Label>(i18n().tr("preferences.applications.mime_types.applications_priority"));
        app_label->set_fixed_size(25);
        right_column->add_child(std::move(app_label));

        auto app_table = std::make_unique<TableView<ApplicationInfo>>();
        m_apps_table = app_table.get();
        m_apps_table->set_header_visible(false);

        TableColumn<ApplicationInfo> col_app;
        col_app.width = 300;
        col_app.cell_factory = [](const ApplicationInfo& info) {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            cell->set_spacing(10);
            
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(info.icon.empty() ? "application-x-executable" : info.icon);
            icon->set_icon_size(16);
            icon->set_fixed_size(16);
            
            cell->add_child(std::move(icon));
            cell->add_child(std::make_unique<Label>(info.name));
            return cell;
        };
        m_apps_table->add_column(col_app);
        right_column->add_child(std::move(app_table));

        auto app_toolbar = std::make_unique<Widget>();
        app_toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        app_toolbar->set_fixed_size(32);
        app_toolbar->set_spacing(5);

        auto create_tool_btn = [this](const std::string& icon_name, std::function<void()> cb) {
            auto btn = std::make_unique<Button<AquaObject>>();
            btn->set_fixed_size(32);
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(icon_name);
            icon->set_icon_size(16);
            btn->add_child(std::move(icon));
            btn->when_click.connect([cb](MouseButtonEventContext&){ cb(); });
            return btn;
        };

        app_toolbar->add_child(create_tool_btn("list-add", [this](){ on_add_app(); }));
        app_toolbar->add_child(create_tool_btn("list-remove", [this](){ on_remove_app(); }));
        app_toolbar->add_child(create_tool_btn("go-up", [this](){ on_move_app_up(); }));
        app_toolbar->add_child(create_tool_btn("go-down", [this](){ on_move_app_down(); }));

        right_column->add_child(std::move(app_toolbar));

        add_child(std::move(right_column));
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

    void MimeTypesView::update_details(const std::string& mime_type)
    {
        if (mime_type.empty()) {
            m_mime_title_label->set_text(i18n().tr("preferences.applications.mime_types.select_mime"));
            m_extensions_table->set_data({});
            m_apps_table->set_data({});
            return;
        }

        m_mime_title_label->set_text("MIME Type: " + mime_type);
        
        if (m_mime_extensions.find(mime_type) == m_mime_extensions.end()) {
            std::vector<MimeExtension> extensions;
            
            auto load_from_dir = [&](const std::string& base_path, bool is_user) {
                std::string xml_path = base_path + mime_type + ".xml";
                std::ifstream file(xml_path);
                if (file.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
                    
                    std::regex glob_re("<glob\\s+pattern=\"([^\"]+)\"");
                    auto words_begin = std::sregex_iterator(content.begin(), content.end(), glob_re);
                    auto words_end = std::sregex_iterator();

                    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                        std::smatch match = *i;
                        std::string pattern = match[1].str();
                        
                        auto it = std::find_if(extensions.begin(), extensions.end(), 
                            [&](const MimeExtension& e) { return e.pattern == pattern; });
                        
                        if (it == extensions.end()) {
                            extensions.push_back({pattern, is_user});
                        } else if (is_user) {
                            it->is_user = true;
                        }
                    }
                }
            };

            load_from_dir("/usr/share/mime/", false);
            
            const char* home = std::getenv("HOME");
            if (home) {
                load_from_dir(std::string(home) + "/.local/share/mime/", true);
            }

            m_mime_extensions[mime_type] = extensions;
        }
        m_extensions_table->set_data(m_mime_extensions[mime_type]);

        // Load applications from system standards
        if (m_mime_apps.find(mime_type) == m_mime_apps.end()) {
            auto entries = DesktopManager::get_apps_for_mime(mime_type);
            std::vector<ApplicationInfo> apps;
            for (const auto& entry : entries) {
                apps.push_back({entry.id, entry.name, entry.icon});
            }
            
            if (apps.empty()) {
                apps.push_back({"text-editor.desktop", i18n().tr("preferences.applications.labels.text_editor"), "text-editor"});
            }
            m_mime_apps[mime_type] = apps;
        }
        m_apps_table->set_data(m_mime_apps[mime_type]);
    }

    void MimeTypesView::on_add_extension()
    {
        if (m_current_mime.empty()) return;
        
        auto dialog = std::make_unique<InputDialog>(
            i18n().tr("preferences.applications.mime_types.add_extension_title"), 
            i18n().tr("preferences.applications.mime_types.add_extension_label"));
        dialog->when_accepted.connect([this](std::string& text) {
            std::string val = text;
            if (auto* app = application()) {
                app->post_task([this, val]() {
                    if (val.empty()) return;
                    std::string pattern = val;
                    if (pattern[0] != '*' && pattern[0] != '.') pattern = "*." + pattern;
                    else if (pattern[0] == '.') pattern = "*" + pattern;

                    const char* home = std::getenv("HOME");
                    if (!home) return;

                    fs::path local_mime_path = fs::path(home) / ".local/share/mime" / m_current_mime;
                    local_mime_path.replace_extension(".xml");
                    
                    fs::create_directories(local_mime_path.parent_path());

                    std::string content;
                    if (fs::exists(local_mime_path)) {
                        std::ifstream infile(local_mime_path);
                        content = std::string((std::istreambuf_iterator<char>(infile)), 
                                            std::istreambuf_iterator<char>());
                    } else {
                        content = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
                        content += "<mime-type xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\" type=\"" + m_current_mime + "\">\n";
                        content += "</mime-type>\n";
                    }

                    size_t pos = content.rfind("</mime-type>");
                    if (pos != std::string::npos) {
                        content.insert(pos, "  <glob pattern=\"" + pattern + "\"/>\n");
                        std::ofstream outfile(local_mime_path);
                        outfile << content;
                    }

                    m_mime_extensions.erase(m_current_mime);
                    update_details(m_current_mime);
                });
            }
        });
        
        std::thread([d = std::move(dialog)]() mutable {
            d->initialize();
            d->run();
        }).detach();
    }

    void MimeTypesView::on_remove_extension()
    {
        int idx = m_extensions_table->selected_index();
        if (idx == -1 || m_current_mime.empty()) return;

        auto extens = m_mime_extensions[m_current_mime];
        if (idx >= (int)extens.size()) return;

        MimeExtension ext = extens[idx];

        if (!ext.is_user) {
            auto dialog = std::make_unique<MessageDialog>(
                i18n().tr("preferences.common.error"), 
                i18n().tr("preferences.applications.mime_types.error_remove_system_ext"),
                MessageType::Warning);
            
            std::thread([d = std::move(dialog)]() mutable {
                d->initialize();
                d->run();
            }).detach();
            return;
        }

        const char* home = std::getenv("HOME");
        if (!home) return;

        fs::path local_mime_path = fs::path(home) / ".local/share/mime" / m_current_mime;
        local_mime_path.replace_extension(".xml");

        if (fs::exists(local_mime_path)) {
            std::ifstream infile(local_mime_path);
            std::string content((std::istreambuf_iterator<char>(infile)), 
                                std::istreambuf_iterator<char>());
            
            std::string tag = "<glob pattern=\"" + ext.pattern + "\"/>";
            size_t tag_pos = content.find(tag);
            if (tag_pos != std::string::npos) {
                content.erase(tag_pos, tag.length());
                if (tag_pos < content.length() && content[tag_pos] == '\n') content.erase(tag_pos, 1);
                
                std::ofstream outfile(local_mime_path);
                outfile << content;
            }
        }

        m_mime_extensions.erase(m_current_mime);
        update_details(m_current_mime);
    }


     void MimeTypesView::on_add_app()
    {
        if (m_current_mime.empty()) return;

        auto dialog = std::make_unique<AppPickerDialog>();
        dialog->when_accepted.connect([this](DesktopEntry& entry) {
            if (auto* app = application()) {
                app->post_task([this, entry]() {
                    // 1. Add to UI if not already present
                    auto& apps = m_mime_apps[m_current_mime];
                    auto it = std::find_if(apps.begin(), apps.end(), [&](const ApplicationInfo& info) {
                        return info.name == entry.name;
                    });
                    
                    if (it == apps.end()) {
                        apps.push_back({entry.id, entry.name, entry.icon});
                        m_apps_table->set_data(apps);
                        
                        // 2. Persist in ~/.config/mimeapps.list
                        DesktopManager::add_mime_association(m_current_mime, entry.id);
                    }
                });
            }
        });
        
        std::thread([d = std::move(dialog)]() mutable {
            d->initialize();
            d->run();
        }).detach();
    }

    void MimeTypesView::on_remove_app()
    {
        int idx = m_apps_table->selected_index();
        if (idx != -1 && !m_current_mime.empty()) {
            auto& v = m_mime_apps[m_current_mime];
            std::string desktop_id = v[idx].id;

            // Final check: only remove if it's not a generic placeholder
            if (!desktop_id.empty()) {
                DesktopManager::remove_mime_association(m_current_mime, desktop_id);
            }

            v.erase(v.begin() + idx);
            m_apps_table->set_data(v);
        }
    }

    void MimeTypesView::on_move_app_up()
    {
        int idx = m_apps_table->selected_index();
        if (idx > 0 && !m_current_mime.empty()) {
            auto& v = m_mime_apps[m_current_mime];
            std::swap(v[idx], v[idx - 1]);
            m_apps_table->set_data(v);
            m_apps_table->set_selected_index(idx - 1);
        }
    }

    void MimeTypesView::on_move_app_down()
    {
        int idx = m_apps_table->selected_index();
        if (idx != -1 && idx < (int)m_mime_apps[m_current_mime].size() - 1 && !m_current_mime.empty()) {
            auto& v = m_mime_apps[m_current_mime];
            std::swap(v[idx], v[idx + 1]);
            m_apps_table->set_data(v);
            m_apps_table->set_selected_index(idx + 1);
        }
    }
} // namespace horizon::preferences
