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
#include <horizon/I18n.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Image.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Application.hpp>

namespace horizon::appstore {

PackageDetailsWidget::PackageDetailsWidget() {
    set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL); // Doesn't matter because we override calculate_layout
    set_position_type(horizon::FILL);

    auto icon = std::make_unique<horizon::Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(64);
    m_icon->set_fixed_size(80);
    m_icon->set_position_type(horizon::FREE);
    add_child(std::move(icon));

    auto title = std::make_unique<horizon::Label>();
    m_title = title.get();
    m_title->set_font_weight(horizon::FONT_WEIGHT_BOLD);
    m_title->set_font_size(16);
    m_title->set_position_type(horizon::FREE);
    add_child(std::move(title));

    auto version = std::make_unique<horizon::Label>();
    m_version = version.get();
    m_version->set_font_size(10);
    m_version->set_position_type(horizon::FREE);
    add_child(std::move(version));

    auto rating = std::make_unique<horizon::ProgressBar>();
    m_rating = rating.get();
    m_rating->set_position_type(horizon::FREE);
    add_child(std::move(rating));

    auto desc = std::make_unique<horizon::Label>();
    m_description = desc.get();
    m_description->set_vertical_alignment(horizon::VerticalAlignment::Top);
    m_description->set_position_type(horizon::FREE);
    add_child(std::move(desc));
}

void PackageDetailsWidget::draw(horizon::GraphicsContext& gc) {
    if (theme_manager()) {
        gc.setColor(theme_manager()->get_color("textbox_bg"));
        gc.fillRect(x(), y(), width(), height());
        
        gc.setColor(theme_manager()->get_color("window_border"));
        gc.drawRect(x(), y(), width(), height(), 0, 1.0f);
    }
    Widget::draw(gc);
}

void PackageDetailsWidget::calculate_layout() {
    if (!m_title) return;
    int w = width();
    if (w <= 0) w = 797; // fallback

    int bx = x();
    int by = y();

    // Icono un poco más grande y con más margen
    m_icon->set_position(bx + 20, by + 20);
    m_icon->set_size(80, 80);
    
    // Título más prominente
    m_title->set_position(bx + 120, by + 20);
    m_title->set_size(w - 120 - 150, 35);
    m_title->set_font_size(24);
    
    // Versión alineada debajo del título
    m_version->set_position(bx + 120, by + 60);
    m_version->set_size(w - 120 - 150, 20);
    m_version->set_font_size(12);

    // Rating (ProgressBar) arriba a la derecha, con un tamaño adecuado
    m_rating->set_position(bx + w - 140, by + 30);
    m_rating->set_size(120, 15);

    // Descripción con un buen margen superior
    m_description->set_position(bx + 20, by + 120);
    int desc_h = m_description->preferred_height(w - 40);
    m_description->set_size(w - 40, desc_h);

    // Capturas de pantalla debajo de la descripción
    int current_y = by + 120 + desc_h + 30;
    int current_x = bx + 20;
    
    for (auto img : m_screenshots) {
        img->set_position(current_x, current_y);
        // Hacer las capturas un poco más grandes para que se vean mejor
        img->set_size(300, 180);
        current_x += 320; // 20px de separación
    }

    int total_height = (current_y - by) + (m_screenshots.empty() ? 0 : 200);
    
    // Ocupar al menos el espacio disponible en el ScrollArea padre
    if (parent()) {
        total_height = std::max(total_height, parent()->height());
    }

    if (height() != total_height) {
        set_height(total_height);
    }
}

void PackageDetailsWidget::update_basic_info(const horizon::apt::PackageInfo& pkg) {
    m_title->set_text(pkg.name);
    m_version->set_text(pkg.version);
    m_description->set_text(pkg.description);
    m_icon->set_icon_name(pkg.icon.empty() ? "system-software-install" : pkg.icon);
    m_rating->set_progress(0.0f); // Default
    clear_screenshots();
    invalidate();
    calculate_layout();
}

void PackageDetailsWidget::update_api_info(const horizon::apt::AppDetails& app_details, const horizon::apt::AppVersionDetails& v_details) {
    if (!v_details.name.empty()) m_title->set_text(v_details.name);
    if (v_details.description && !v_details.description->empty()) m_description->set_text(*v_details.description);
    
    // Rating mapping from 0 to 5 -> 0.0f to 1.0f
    m_rating->set_progress(app_details.avg_rating / 5.0f);
    
    invalidate();
    calculate_layout();
}

void PackageDetailsWidget::add_screenshot(const std::string& local_path) {
    auto img = std::make_unique<horizon::Image>();
    img->set_path(local_path);
    img->set_mode(horizon::ImageMode::Fit);
    img->set_position_type(horizon::FREE);
    m_screenshots.push_back(img.get());
    add_child(std::move(img));
    invalidate();
    calculate_layout();
}

void PackageDetailsWidget::clear_screenshots() {
    for (auto img : m_screenshots) {
        remove_child(img);
    }
    m_screenshots.clear();
    invalidate();
    calculate_layout();
}

ExploreView::ExploreView(horizon::apt::AptManager* apt_manager) : m_apt(apt_manager) {
    set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    setup_ui();
    build_categories();
}

class NoSelectionWidget : public horizon::Widget {
public:
    NoSelectionWidget() {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::FILL);

        auto lbl = std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.explore.no_selection"));
        lbl->set_alignment(horizon::TextAlignment::Center);
        lbl->set_vertical_alignment(horizon::VerticalAlignment::Middle);
        lbl->set_font_size(18);
        add_child(std::move(lbl));
    }

    void draw(horizon::GraphicsContext& gc) override {
        if (theme_manager()) {
            gc.setColor(theme_manager()->get_color("textbox_bg"));
            gc.fillRect(x(), y(), width(), height());
            
            gc.setColor(theme_manager()->get_color("window_border"));
            gc.drawRect(x(), y(), width(), height(), 0, 1.0f);
        }
        Widget::draw(gc);
    }
};

void ExploreView::clear_selection() {
    m_selected_pkg = std::nullopt;
    if (m_no_sel_widget) m_no_sel_widget->set_visible(true);
    if (m_details_scroll) m_details_scroll->set_visible(false);
    if (on_package_selected) on_package_selected(nullptr);
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
    col_cat_name.title = horizon::i18n().tr("appstore.explore.column.category");
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
    col_app.title = horizon::i18n().tr("appstore.explore.column.application");
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
    col_version.title = horizon::i18n().tr("appstore.explore.column.version");
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
    auto scroll_area = std::make_unique<horizon::ScrollArea>();
    m_details_scroll = scroll_area.get();
    m_details_scroll->set_fixed_size(250); // initial height, can be overridden by layout
    
    auto details_widget = std::make_unique<PackageDetailsWidget>();
    m_details_widget = details_widget.get();
    m_details_scroll->set_content(std::move(details_widget));

    auto no_sel_widget = std::make_unique<NoSelectionWidget>();
    m_no_sel_widget = no_sel_widget.get();

    m_details_scroll->set_visible(false);
    m_no_sel_widget->set_visible(true);

    right_panel->add_child(std::move(scroll_area));
    right_panel->add_child(std::move(no_sel_widget));

    m_vpanel->add_child(std::move(right_panel));
    add_child(std::move(vpanel));
}

void ExploreView::build_categories() {
    std::vector<CategoryItem> categories = {
        {horizon::i18n().tr("appstore.category.all"), "applications-all"},
        {horizon::i18n().tr("appstore.category.accessories"), "applications-utilities"},
        {horizon::i18n().tr("appstore.category.education"), "applications-science"},
        {horizon::i18n().tr("appstore.category.graphics"), "applications-graphics"},
        {horizon::i18n().tr("appstore.category.internet"), "applications-internet"},
        {horizon::i18n().tr("appstore.category.games"), "applications-games"},
        {horizon::i18n().tr("appstore.category.multimedia"), "applications-multimedia"},
        {horizon::i18n().tr("appstore.category.office"), "applications-office"},
        {horizon::i18n().tr("appstore.category.development"), "applications-development"},
        {horizon::i18n().tr("appstore.category.system"), "applications-system"},
        {horizon::i18n().tr("appstore.category.other"), "applications-other"}
    };

    m_category_table->set_data(std::move(categories));
}

void ExploreView::load_initial_data() {
    filter_by_category(horizon::i18n().tr("appstore.category.all"));
}

void ExploreView::perform_search(const std::string& query) {
    m_current_search_query = query;
    m_current_category = "";
    if (m_apt && !query.empty()) {
        if (on_loading_state_changed) {
            std::string msg = horizon::i18n().tr("appstore.status.searching");
            auto pos = msg.find("%1");
            if (pos != std::string::npos) msg.replace(pos, 2, query);
            on_loading_state_changed(true, msg);
        }

        std::thread([this, query]() {
            auto results = m_apt->search_packages(query);
            if (application()) {
                application()->post_task([this, query, results = std::move(results)]() mutable {
                    if (on_loading_state_changed) {
                        on_loading_state_changed(false, horizon::i18n().tr("appstore.status.search_finished"));
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
                    } else {
                        clear_selection();
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
        std::string cat_display = category_name.empty() ? horizon::i18n().tr("appstore.category.all") : category_name;
        std::string msg = horizon::i18n().tr("appstore.status.loading_category");
        auto pos = msg.find("%1");
        if (pos != std::string::npos) msg.replace(pos, 2, cat_display);
        on_loading_state_changed(true, msg);
    }

    std::thread([this, category_name]() {
        std::vector<std::string> sections;
        if (category_name == horizon::i18n().tr("appstore.category.accessories")) {
            sections = {"utils", "misc"};
        } else if (category_name == horizon::i18n().tr("appstore.category.education")) {
            sections = {"education", "math", "science"};
        } else if (category_name == horizon::i18n().tr("appstore.category.graphics")) {
            sections = {"graphics"};
        } else if (category_name == horizon::i18n().tr("appstore.category.internet")) {
            sections = {"web", "net", "mail", "news"};
        } else if (category_name == horizon::i18n().tr("appstore.category.games")) {
            sections = {"games"};
        } else if (category_name == horizon::i18n().tr("appstore.category.multimedia")) {
            sections = {"sound", "video", "media"};
        } else if (category_name == horizon::i18n().tr("appstore.category.office")) {
            sections = {"editors", "text", "doc"};
        } else if (category_name == horizon::i18n().tr("appstore.category.development")) {
            sections = {"devel", "lisp", "java", "python", "ruby", "perl", "rust", "golang"};
        } else if (category_name == horizon::i18n().tr("appstore.category.system")) {
            sections = {"admin", "sys"};
        } else if (category_name == horizon::i18n().tr("appstore.category.other")) {
            sections = {"other"};
        }

        auto results = m_apt->list_packages_by_sections(sections, 50);
        
        if (application()) {
            application()->post_task([this, results = std::move(results)]() mutable {
                m_tableview->set_data(std::move(results));
                clear_selection();
                if (on_loading_state_changed) {
                    on_loading_state_changed(false, horizon::i18n().tr("appstore.status.ready"));
                }
            });
        }
    }).detach();
}

void ExploreView::update_details(const horizon::apt::PackageInfo& pkg) {
    if (m_no_sel_widget) m_no_sel_widget->set_visible(false);
    if (m_details_scroll) m_details_scroll->set_visible(true);

    m_selected_pkg = pkg;
    m_details_widget->update_basic_info(pkg);
    
    if (on_package_selected) {
        on_package_selected(&(*m_selected_pkg));
    }

    std::string pkg_name = pkg.name;
    m_api_client.get_app_details_async(pkg_name, "es", [this, pkg_name](std::optional<horizon::apt::AppDetails> details) {
        if (details) {
            if (!details->versions.empty()) {
                std::string latest_version = details->versions[0].version_string;
                m_api_client.get_app_version_details_async(pkg_name, latest_version, "es", [this, pkg_name, details_val = *details](std::optional<horizon::apt::AppVersionDetails> v_details) {
                    if (v_details) {
                        if (application()) {
                            application()->post_task([this, pkg_name, details_val, v_details = *v_details]() {
                                if (m_selected_pkg && m_selected_pkg->name == pkg_name) {
                                    m_details_widget->update_api_info(details_val, v_details);
                                    if (v_details.icon_path && !v_details.icon_path->empty()) {
                                        m_api_client.download_image_async(*v_details.icon_path, [this, pkg_name](std::optional<std::string> local_path) {
                                            if (local_path && application()) {
                                                application()->post_task([this, pkg_name, path = *local_path]() {
                                                    if (m_selected_pkg && m_selected_pkg->name == pkg_name) {
                                                        m_details_widget->icon()->set_icon_path(path);
                                                    }
                                                });
                                            }
                                        });
                                    }
                                    
                                    // Descargar screenshots (max 3)
                                    int count = 0;
                                    for (const auto& shot : v_details.screenshots) {
                                        if (count >= 3) break;
                                        m_api_client.download_image_async(shot.image_path, [this, pkg_name](std::optional<std::string> local_path) {
                                            if (local_path && application()) {
                                                application()->post_task([this, pkg_name, path = *local_path]() {
                                                    if (m_selected_pkg && m_selected_pkg->name == pkg_name) {
                                                        m_details_widget->add_screenshot(path);
                                                    }
                                                });
                                            }
                                        });
                                        count++;
                                    }
                                }
                            });
                        }
                    }
                });
            }
        }
    });
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
        std::string msg = horizon::i18n().tr("appstore.status.installing");
        auto pos = msg.find("%1");
        if (pos != std::string::npos) msg.replace(pos, 2, pkg_name);
        on_loading_state_changed(true, msg);
    }
    
    std::thread([this, pkg_name]() {
        bool success = m_apt->install_package(pkg_name);
        m_apt->reload_cache(); // Refresh cache with new package status
        application()->post_task([this, success, pkg_name]() {
            if (on_loading_state_changed) {
                std::string msg;
                if (success) {
                    msg = horizon::i18n().tr("appstore.status.install_success");
                } else {
                    msg = horizon::i18n().tr("appstore.status.install_error");
                    auto pos = msg.find("%1");
                    if (pos != std::string::npos) msg.replace(pos, 2, pkg_name);
                }
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
        std::string msg = horizon::i18n().tr("appstore.status.uninstalling");
        auto pos = msg.find("%1");
        if (pos != std::string::npos) msg.replace(pos, 2, pkg_name);
        on_loading_state_changed(true, msg);
    }
    
    std::thread([this, pkg_name]() {
        bool success = m_apt->remove_package(pkg_name);
        m_apt->reload_cache(); // Refresh cache with new package status
        application()->post_task([this, success, pkg_name]() {
            if (on_loading_state_changed) {
                std::string msg;
                if (success) {
                    msg = horizon::i18n().tr("appstore.status.uninstall_success");
                } else {
                    msg = horizon::i18n().tr("appstore.status.uninstall_error");
                    auto pos = msg.find("%1");
                    if (pos != std::string::npos) msg.replace(pos, 2, pkg_name);
                }
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
