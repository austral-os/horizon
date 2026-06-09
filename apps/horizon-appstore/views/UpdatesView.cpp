#include "UpdatesView.hpp"
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/I18n.hpp>
#include <thread>

namespace horizon::appstore {

UpdatesView::UpdatesView(horizon::apt::AptManager* apt_manager) : m_apt(apt_manager) {
    set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    setup_ui();
}

void UpdatesView::setup_ui() {
    auto container = std::make_unique<horizon::Widget>();
    container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);

    // --- Empty State ---
    auto empty_state = std::make_unique<horizon::Widget>();
    m_empty_state = empty_state.get();
    m_empty_state->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    
    auto empty_vbox = std::make_unique<horizon::Widget>();
    empty_vbox->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    empty_vbox->set_margin(50);
    
    auto icon = std::make_unique<horizon::Icon>();
    icon->set_icon_name("safety-symbolic");
    icon->set_icon_size(128);
    icon->set_fixed_size(150);
    
    auto title = std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.updates.empty_title"));
    title->set_font_weight(horizon::FONT_WEIGHT_BOLD);
    title->set_alignment(horizon::TextAlignment::Center);
    title->set_font_size(24);
    
    auto desc = std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.updates.empty_desc"));
    desc->set_alignment(horizon::TextAlignment::Center);
    
    auto btn_wrapper = std::make_unique<horizon::Widget>();
    btn_wrapper->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);

    auto chk_container = std::make_unique<horizon::Widget>();
    chk_container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    chk_container->set_fixed_size(200);
    
    auto btn_check = std::make_unique<horizon::Button<horizon::AquaObject>>();
    btn_check->set_text(horizon::i18n().tr("appstore.updates.check_btn"));
    btn_check->set_accent_color(horizon::WidgetAccentColor::Primary);
    btn_check->set_fixed_size(35);
    m_btn_check = btn_check.get();
    m_btn_check->when_click.connect([this](auto&) {
        check_for_updates();
    });

    chk_container->add_child(std::move(btn_check));

    btn_wrapper->add_child(horizon::Spacer());
    btn_wrapper->add_child(std::move(chk_container));
    btn_wrapper->add_child(horizon::Spacer());

    empty_vbox->add_child(horizon::Spacer());
    empty_vbox->add_child(std::move(icon));
    empty_vbox->add_child(std::move(title));
    empty_vbox->add_child(horizon::Spacer(10));
    empty_vbox->add_child(std::move(desc));
    empty_vbox->add_child(horizon::Spacer(20));
    empty_vbox->add_child(std::move(btn_wrapper));
    empty_vbox->add_child(horizon::Spacer());

    m_empty_state->add_child(std::move(empty_vbox));
    
    // --- Results State ---
    auto results_state = std::make_unique<horizon::Widget>();
    m_results_state = results_state.get();
    m_results_state->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    m_results_state->set_visible(false);

    auto tableview = std::make_unique<horizon::TableView<horizon::apt::PackageInfo>>();
    m_tableview = tableview.get();
    m_tableview->set_row_height(56);

    horizon::TableColumn<horizon::apt::PackageInfo> col_app;
    col_app.title = horizon::i18n().tr("appstore.explore.column.application");
    col_app.width = 400;
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
    col_version.width = 200;
    col_version.cell_factory = [](const horizon::apt::PackageInfo& pkg) -> std::unique_ptr<horizon::Widget> {
        auto container = std::make_unique<horizon::Widget>();
        container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(5);
        container->set_margin(5);
        
        auto lbl = std::make_unique<horizon::Label>(pkg.version);
        lbl->set_alignment(horizon::TextAlignment::Right);
        lbl->set_vertical_alignment(horizon::VerticalAlignment::Middle);

        container->add_child(std::move(lbl));
        return container;
    };
    m_tableview->add_column(col_version);

    m_results_state->add_child(std::move(tableview));

    container->add_child(std::move(empty_state));
    container->add_child(std::move(results_state));
    add_child(std::move(container));
}

void UpdatesView::load_initial_data() {
    check_for_updates();
}

void UpdatesView::check_for_updates() {
    if (!m_apt) return;
    
    if (on_loading_state_changed) {
        on_loading_state_changed(true, horizon::i18n().tr("appstore.status.searching"));
    }
    
    std::thread([this]() {
        auto results = m_apt->list_upgradable_packages();
        
        if (application()) {
            application()->post_task([this, results = std::move(results)]() mutable {
                if (on_loading_state_changed) {
                    on_loading_state_changed(false, horizon::i18n().tr("appstore.status.ready"));
                }
                
                bool has_updates = !results.empty();
                m_empty_state->set_visible(!has_updates);
                m_results_state->set_visible(has_updates);
                
                if (has_updates) {
                    m_tableview->set_data(std::move(results));
                }
                
                if (on_updates_status_changed) {
                    on_updates_status_changed(has_updates ? m_tableview->data().size() : 0);
                }
            });
        }
    }).detach();
}

void UpdatesView::trigger_update_all() {
    if (!m_apt) return;
    
    if (on_loading_state_changed) {
        on_loading_state_changed(true, horizon::i18n().tr("appstore.status.updating"));
    }
    
    std::thread([this]() {
        bool success = m_apt->upgrade_all_packages();
        m_apt->reload_cache();
        
        if (application()) {
            application()->post_task([this, success]() {
                if (on_loading_state_changed) {
                    std::string msg = success ? horizon::i18n().tr("appstore.status.update_success") : horizon::i18n().tr("appstore.status.update_error");
                    on_loading_state_changed(false, msg);
                }
                
                check_for_updates(); // Refresh view
            });
        }
    }).detach();
}

}
