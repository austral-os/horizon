#include "ExploreView.hpp"
#include <horizon/TableColumn.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Checkbox.hpp>
#include <iostream>
#include <horizon/AquaObject.hpp>
#include <horizon/apt/AptManager.hpp>
#include <thread>

namespace horizon::appstore {

ExploreView::ExploreView(horizon::apt::AptManager* apt_manager) : m_apt(apt_manager) {
    set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    setup_ui();
    build_categories();
}

void ExploreView::setup_ui() {
    auto vpanel = std::make_unique<horizon::VPanel>();
    m_vpanel = vpanel.get();

    // Left Panel: Categories Table
    auto cat_table = std::make_unique<horizon::TableView<CategoryItem>>();
    m_category_table = cat_table.get();
    m_category_table->set_fixed_size(250); // width
    m_category_table->set_header_visible(false);

    horizon::TableColumn<CategoryItem> col_cat_icon;
    col_cat_icon.title = "";
    col_cat_icon.width = 32;
    col_cat_icon.sortable = false;
    col_cat_icon.cell_factory = [](const CategoryItem& c) -> std::unique_ptr<horizon::Widget> {
        auto icon = std::make_unique<horizon::Icon>();
        icon->set_icon_name(c.icon);
        icon->set_icon_size(16);
        return icon;
    };
    m_category_table->add_column(col_cat_icon);

    horizon::TableColumn<CategoryItem> col_cat_name;
    col_cat_name.title = "Categoría";
    col_cat_name.width = 200;
    col_cat_name.cell_factory = [](const CategoryItem& c) -> std::unique_ptr<horizon::Widget> {
        return std::make_unique<horizon::Label>(c.name);
    };
    m_category_table->add_column(col_cat_name);

    m_category_table->when_row_click.connect([this](const horizon::TableViewRowMouseClickContext<CategoryItem>& ctx) {
        filter_by_category(ctx.row_data.name);
    });

    m_vpanel->add_child(std::move(cat_table));

    // Right Panel: Results & Details
    auto right_panel = std::make_unique<horizon::Widget>();
    right_panel->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);

    auto tableview = std::make_unique<horizon::TableView<horizon::apt::PackageInfo>>();
    m_tableview = tableview.get();
    m_tableview->set_row_height(56);

    horizon::TableColumn<horizon::apt::PackageInfo> col_installed;
    col_installed.title = "";
    col_installed.width = 40;
    col_installed.sortable = false;
    col_installed.cell_factory = [](const horizon::apt::PackageInfo& pkg) -> std::unique_ptr<horizon::Widget> {
        auto container = std::make_unique<horizon::Widget>();
        container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        
        auto chk = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        chk->set_checked(pkg.is_installed);
        chk->set_enabled(false);
        container->add_child(std::move(chk));

        container->add_child(horizon::Spacer());
        return container;
    };
    m_tableview->add_column(col_installed);

    horizon::TableColumn<horizon::apt::PackageInfo> col_app;
    col_app.title = "Aplicación";
    col_app.width = 300;
    col_app.cell_factory = [](const horizon::apt::PackageInfo& pkg) -> std::unique_ptr<horizon::Widget> {
        auto hpanel = std::make_unique<horizon::Widget>();
        hpanel->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        hpanel->set_margin(5);
        hpanel->set_spacing(5);
        
        auto icon = std::make_unique<horizon::Icon>();
        icon->set_icon_name(pkg.icon.empty() ? "system-software-install" : pkg.icon);
        icon->set_icon_size(24);
        icon->set_fixed_size(32);
        icon->set_vertical_alignment(horizon::VerticalAlignment::Top);
        
        auto vpanel = std::make_unique<horizon::Widget>();
        vpanel->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        
        auto name_lbl = std::make_unique<horizon::Label>(pkg.name);
        name_lbl->set_font_weight(horizon::FONT_WEIGHT_BOLD);
        name_lbl->set_vertical_alignment(horizon::VerticalAlignment::Top);
        
        auto desc_lbl = std::make_unique<horizon::Label>(pkg.description);
        desc_lbl->set_font_size(10);
        desc_lbl->set_vertical_alignment(horizon::VerticalAlignment::Top);
        
        vpanel->add_child(std::move(name_lbl));
        vpanel->add_child(std::move(desc_lbl));
        
        hpanel->add_child(std::move(icon));
        hpanel->add_child(std::move(vpanel));
        return hpanel;
    };
    m_tableview->add_column(col_app);

    horizon::TableColumn<horizon::apt::PackageInfo> col_version;
    col_version.title = "Versión";
    col_version.width = 150;
    col_version.cell_factory = [](const horizon::apt::PackageInfo& pkg) -> std::unique_ptr<horizon::Widget> {
        std::string clean_ver = pkg.version;
        auto colon_pos = clean_ver.find(':');
        if (colon_pos != std::string::npos && colon_pos < 3) {
            clean_ver = clean_ver.substr(colon_pos + 1);
        }
        auto dash_pos = clean_ver.find('-');
        if (dash_pos != std::string::npos) {
            clean_ver = clean_ver.substr(0, dash_pos);
        }
        auto plus_pos = clean_ver.find('+');
        if (plus_pos != std::string::npos) {
            clean_ver = clean_ver.substr(0, plus_pos);
        }
        if (!clean_ver.empty() && clean_ver[0] != 'v') {
            clean_ver = "v" + clean_ver;
        }

        auto container = std::make_unique<horizon::Widget>();
        container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(5);
        container->set_margin(5);
        
        auto lbl = std::make_unique<horizon::Label>(clean_ver);
        lbl->set_alignment(horizon::TextAlignment::Right);
        lbl->set_vertical_alignment(horizon::VerticalAlignment::Top);

        container->add_child(std::move(lbl));
        return container;
    };
    m_tableview->add_column(col_version);

    m_tableview->when_row_click.connect([this](const horizon::TableViewRowMouseClickContext<horizon::apt::PackageInfo>& ctx) {
        update_details(ctx.row_data);
    });

    right_panel->add_child(std::move(tableview));

    // Details Panel
    auto details_panel = std::make_unique<horizon::Widget>();
    details_panel->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
    details_panel->set_fixed_size(150); // height

    auto icon = std::make_unique<horizon::Icon>();
    m_detail_icon = icon.get();
    m_detail_icon->set_icon_size(64);
    m_detail_icon->set_fixed_size(80);
    m_detail_icon->set_vertical_alignment(horizon::VerticalAlignment::Top);
    details_panel->add_child(std::move(icon));

    auto info_panel = std::make_unique<horizon::Widget>();
    info_panel->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);

    auto title = std::make_unique<horizon::Label>("");
    m_detail_title = title.get();
    m_detail_title->set_fixed_size(30); // height
    m_detail_title->set_font_weight(horizon::FONT_WEIGHT_BOLD);
    info_panel->add_child(std::move(title));

    auto desc = std::make_unique<horizon::Label>("");
    m_detail_desc = desc.get();
    m_detail_desc->set_vertical_alignment(horizon::VerticalAlignment::Top);
    info_panel->add_child(std::move(desc));

    details_panel->add_child(std::move(info_panel));
    right_panel->add_child(std::move(details_panel));

    m_vpanel->add_child(std::move(right_panel));
    add_child(std::move(vpanel));
}

void ExploreView::build_categories() {
    std::vector<CategoryItem> categories = {
        {"Todas", "applications-all"},
        {"Accesorios", "applications-utilities"},
        {"Educación", "applications-science"},
        {"Gráficos", "applications-graphics"},
        {"Internet", "applications-internet"},
        {"Juegos", "applications-games"},
        {"Multimedia", "applications-multimedia"},
        {"Ofimática", "applications-office"},
        {"Desarrollo", "applications-development"},
        {"Sistema", "applications-system"},
        {"Otros", "applications-other"}
    };

    m_category_table->set_data(std::move(categories));
}

void ExploreView::load_initial_data() {
    filter_by_category("Todas");
}

void ExploreView::perform_search(const std::string& query) {
    m_current_search_query = query;
    m_current_category = "";
    if (m_apt && !query.empty()) {
        if (on_loading_state_changed) {
            on_loading_state_changed(true, "Buscando '" + query + "'...");
        }

        std::thread([this, query]() {
            auto results = m_apt->search_packages(query);
            if (application()) {
                application()->post_task([this, query, results = std::move(results)]() mutable {
                    if (on_loading_state_changed) {
                        on_loading_state_changed(false, "Búsqueda finalizada");
                    }
                    
                    bool has_results = !results.empty();
                    int best_match_idx = 0;
                    if (has_results) {
                        for (size_t i = 0; i < results.size(); ++i) {
                            if (results[i].name == query) {
                                best_match_idx = i;
                                break;
                            }
                        }
                    }

                    std::cout << "DEBUG: Search query='" << query << "' found at index " << best_match_idx << " total results=" << results.size() << std::endl;

                    m_tableview->set_data(results);
                    
                    if (has_results) {
                        m_tableview->set_selected_index(best_match_idx);
                        m_tableview->scroll_to_index(best_match_idx);
                        update_details(results[best_match_idx]);
                    }
                });
            }
        }).detach();
    }
}

void ExploreView::filter_by_category(const std::string& category_name) {
    m_current_category = category_name;
    m_current_search_query = "";
    if (!m_apt) return;

    if (on_loading_state_changed) {
        on_loading_state_changed(true, "Cargando " + (category_name.empty() ? "Todas" : category_name) + "...");
    }

    std::thread([this, category_name]() {
        std::vector<std::string> sections;
        if (category_name == "Accesorios") {
            sections = {"utils", "misc"};
        } else if (category_name == "Educación") {
            sections = {"education", "math", "science"};
        } else if (category_name == "Gráficos") {
            sections = {"graphics"};
        } else if (category_name == "Internet") {
            sections = {"web", "net", "mail", "news"};
        } else if (category_name == "Juegos") {
            sections = {"games"};
        } else if (category_name == "Multimedia") {
            sections = {"sound", "video", "media"};
        } else if (category_name == "Ofimática") {
            sections = {"editors", "text", "doc"};
        } else if (category_name == "Desarrollo") {
            sections = {"devel", "lisp", "java", "python", "ruby", "perl", "rust", "golang"};
        } else if (category_name == "Sistema") {
            sections = {"admin", "sys"};
        } else if (category_name == "Otros") {
            sections = {"other"};
        }

        auto results = m_apt->list_packages_by_sections(sections, 50);
        
        if (application()) {
            application()->post_task([this, results = std::move(results)]() mutable {
                m_tableview->set_data(std::move(results));
                if (on_loading_state_changed) {
                    on_loading_state_changed(false, "Listo");
                }
            });
        }
    }).detach();
}

void ExploreView::update_details(const horizon::apt::PackageInfo& pkg) {
    m_selected_pkg = pkg;
    m_detail_title->set_text(pkg.name);
    m_detail_desc->set_text(pkg.description);
    m_detail_icon->set_icon_name(pkg.icon.empty() ? "system-software-install" : pkg.icon);
    
    if (on_package_selected) {
        on_package_selected(&(*m_selected_pkg));
    }
}

void ExploreView::reload_current_view() {
    if (!m_current_search_query.empty()) {
        perform_search(m_current_search_query);
    } else if (!m_current_category.empty()) {
        filter_by_category(m_current_category);
    } else {
        load_initial_data();
    }
}

void ExploreView::trigger_install() {
    if (!m_selected_pkg) return;
    std::string pkg_name = m_selected_pkg->name;
    
    if (on_loading_state_changed) {
        on_loading_state_changed(true, "Instalando " + pkg_name + "...");
    }
    
    std::thread([this, pkg_name]() {
        bool success = m_apt->install_package(pkg_name);
        m_apt->reload_cache(); // Refresh cache with new package status
        application()->post_task([this, success, pkg_name]() {
            if (on_loading_state_changed) {
                std::string msg = success ? "Instalación completada." : "Error instalando " + pkg_name + ".";
                on_loading_state_changed(false, msg);
            }
            // Update selected package status manually if it matches
            if (success && m_selected_pkg && m_selected_pkg->name == pkg_name) {
                m_selected_pkg->is_installed = true;
                if (on_package_selected) on_package_selected(&(*m_selected_pkg));
            }
            reload_current_view();
        });
    }).detach();
}

void ExploreView::trigger_remove() {
    if (!m_selected_pkg) return;
    std::string pkg_name = m_selected_pkg->name;
    
    if (on_loading_state_changed) {
        on_loading_state_changed(true, "Desinstalando " + pkg_name + "...");
    }
    
    std::thread([this, pkg_name]() {
        bool success = m_apt->remove_package(pkg_name);
        m_apt->reload_cache(); // Refresh cache with new package status
        application()->post_task([this, success, pkg_name]() {
            if (on_loading_state_changed) {
                std::string msg = success ? "Desinstalación completada." : "Error desinstalando " + pkg_name + ".";
                on_loading_state_changed(false, msg);
            }
            // Update selected package status manually if it matches
            if (success && m_selected_pkg && m_selected_pkg->name == pkg_name) {
                m_selected_pkg->is_installed = false;
                if (on_package_selected) on_package_selected(&(*m_selected_pkg));
            }
            reload_current_view();
        });
    }).detach();
}

}
