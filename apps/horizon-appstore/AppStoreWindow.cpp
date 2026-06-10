#include "AppStoreWindow.hpp"
#include <horizon/Toolbar.hpp>
#include <horizon/Statusbar.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Label.hpp>

namespace horizon::appstore {

AppStoreWindow::AppStoreWindow(const std::string& initial_view, const std::string& initial_search) : ApplicationWindow("AppStore") {
    set_size(1000, 700);
    
    m_apt_manager = std::make_shared<horizon::apt::AptManager>();
    m_apt_manager->initialize();

    setup_content(initial_search);
    setup_toolbar();
    setup_statusbar();
    
    set_active_view(initial_view);
    
    if (!initial_search.empty() && m_search_box) {
        m_search_box->set_text(initial_search);
    }
}

void AppStoreWindow::set_active_view(const std::string& view_name) {
    if (!m_group_btn) return;
    
    if (view_name == horizon::i18n().tr("appstore.views.explore")) {
        m_group_btn->set_current_item(1);
    } else if (view_name == horizon::i18n().tr("appstore.views.updates")) {
        m_group_btn->set_current_item(2);
    } else {
        m_group_btn->set_current_item(0);
    }

    bool is_explore = (view_name == horizon::i18n().tr("appstore.views.explore"));
    bool is_updates = (view_name == horizon::i18n().tr("appstore.views.updates"));
    
    if (m_featured_view) m_featured_view->set_visible(view_name == horizon::i18n().tr("appstore.views.featured"));
    if (m_explore_view) m_explore_view->set_visible(is_explore);
    if (m_updates_view) m_updates_view->set_visible(is_updates);
    
    if (m_btn_action) {
        if (is_explore && m_explore_view && m_explore_view->selected_package()) {
            m_btn_action->set_visible(true);
        } else {
            m_btn_action->set_visible(false);
        }
    }

    if (m_btn_update_all) {
        if (is_updates && m_has_updates) {
            m_btn_update_all->set_visible(true);
        } else {
            m_btn_update_all->set_visible(false);
        }
    }

    if (m_btn_refresh_featured) {
        m_btn_refresh_featured->set_visible(view_name == horizon::i18n().tr("appstore.views.featured"));
    }
}

void AppStoreWindow::setup_toolbar() {
    auto tb = toolbar();

    auto btn_wrapper = std::make_unique<horizon::Widget>();
    
    auto btn_action = std::make_unique<horizon::ToolbarButton>(horizon::i18n().tr("appstore.action.install"), "system-software-install");
    m_btn_action = btn_action.get();
    m_btn_action->set_fixed_size(45);
    m_btn_action->set_visible(false);
    m_btn_action->when_click.connect([this](auto&) {
        if (!m_explore_view) return;
        const auto* pkg = m_explore_view->selected_package();
        if (!pkg) return;
        if (pkg->is_installed) {
            m_explore_view->trigger_remove();
        } else {
            m_explore_view->trigger_install();
        }
    });
    btn_wrapper->add_child(std::move(btn_action));
    auto btn_update_all = std::make_unique<horizon::ToolbarButton>(horizon::i18n().tr("appstore.action.update_all"), "software-update-available-symbolic");
    m_btn_update_all = btn_update_all.get();
    m_btn_update_all->set_fixed_size(45);
    m_btn_update_all->set_visible(false);
    m_btn_update_all->when_click.connect([this](auto&) {
        if (m_updates_view) m_updates_view->trigger_update_all();
    });
    btn_wrapper->add_child(std::move(btn_update_all));

    auto btn_refresh_featured = std::make_unique<horizon::ToolbarButton>(horizon::i18n().tr("appstore.action.refresh"), "view-refresh");
    m_btn_refresh_featured = btn_refresh_featured.get();
    m_btn_refresh_featured->set_fixed_size(45);
    m_btn_refresh_featured->set_visible(false);
    m_btn_refresh_featured->when_click.connect([this](auto&) {
        if (m_featured_view) m_featured_view->reload_data();
    });
    btn_wrapper->add_child(std::move(btn_refresh_featured));

    tb->add_toolbar_widget(std::move(btn_wrapper));
    
    tb->add_toolbar_widget(horizon::Spacer());

    auto group_btn = std::make_unique<horizon::ToggleGroupButton>();
    m_group_btn = group_btn.get();
    m_group_btn->add_item(horizon::i18n().tr("appstore.views.featured"));
    m_group_btn->add_item(horizon::i18n().tr("appstore.views.explore"));
    m_group_btn->add_item(horizon::i18n().tr("appstore.views.updates"));
    m_group_btn->set_fixed_size(400);
    m_group_btn->set_current_item(0);

    m_group_btn->when_button_clicked.connect([this](horizon::GroupButtonClickEvent& ctx) {
        set_active_view(ctx.button_text);
    });

    tb->add_toolbar_widget(horizon::Spacer());
    tb->add_toolbar_widget(std::move(group_btn));
    tb->add_toolbar_widget(horizon::Spacer());

    auto search_box = std::make_unique<horizon::SearchBox>();
    m_search_box = search_box.get();
    m_search_box->set_fixed_size(35);
    m_search_box->set_placeholder(horizon::i18n().tr("appstore.search.placeholder"));
    m_search_box->when_text_changed.connect([this](horizon::KeyEventContext&) {
        static size_t debounce_timer = 0;
        if (debounce_timer) application()->stop_timer(debounce_timer);
        debounce_timer = application()->add_timer(300, [this]() {
            if (m_explore_view) {
                m_explore_view->perform_search(m_search_box->text());
                // Switch to explore view if we are searching
                m_group_btn->set_current_item(1);
                set_active_view(horizon::i18n().tr("appstore.views.explore"));
            }
        });
    });
    
    auto search_wrapper = std::make_unique<horizon::Widget>();
    search_wrapper->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    search_wrapper->set_fixed_size(200);
    search_wrapper->add_child(std::move(search_box));
    
    tb->add_toolbar_widget(std::move(search_wrapper));
    tb->add_toolbar_widget(horizon::Spacer(10));
}

void AppStoreWindow::setup_content(const std::string& initial_search) {
    auto content = std::make_unique<horizon::Widget>();
    m_content_area = content.get();
    m_content_area->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    
    auto featured = std::make_unique<FeaturedView>();
    m_featured_view = featured.get();
    m_featured_view->when_app_clicked.connect([this](const FeaturedView::AppClickedContext& ctx) {
        if (m_search_box) {
            m_search_box->set_text(ctx.package_name);
        }
        if (m_group_btn) {
            m_group_btn->set_current_item(1);
        }
        set_active_view(horizon::i18n().tr("appstore.views.explore"));
        if (m_explore_view) {
            m_explore_view->perform_search(ctx.package_name);
        }
    });
    
    m_featured_view->when_category_clicked.connect([this](const FeaturedView::CategoryClickedContext& ctx) {
        if (m_group_btn) {
            m_group_btn->set_current_item(1);
        }
        set_active_view(horizon::i18n().tr("appstore.views.explore"));
        if (m_explore_view) {
            if (m_search_box) m_search_box->set_text("");
            m_explore_view->select_category(ctx.category_name);
        }
    });

    m_content_area->add_child(std::move(featured));
    
    auto explore = std::make_unique<ExploreView>(m_apt_manager);
    m_explore_view = explore.get();
    m_explore_view->set_visible(false);
    m_explore_view->on_loading_state_changed = [this](bool loading, const std::string& msg) {
        set_status(msg, loading);
        if (m_btn_action) m_btn_action->set_enabled(!loading);
    };
    
    m_explore_view->on_package_selected = [this](const horizon::apt::PackageInfo* pkg) {
        if (!m_btn_action) return;
        if (pkg) {
            // Only show if we are actually in the explore view
            if (m_group_btn && m_group_btn->current_item() == 1) {
                m_btn_action->set_visible(true);
            }
            if (pkg->is_installed) {
                m_btn_action->set_title(horizon::i18n().tr("appstore.action.uninstall"));
                m_btn_action->set_icon_name("edit-delete");
            } else {
                m_btn_action->set_title(horizon::i18n().tr("appstore.action.install"));
                m_btn_action->set_icon_name("system-software-install");
            }
        } else {
            m_btn_action->set_visible(false);
        }
    };
    
    m_content_area->add_child(std::move(explore));
    
    auto updates = std::make_unique<UpdatesView>(m_apt_manager);
    m_updates_view = updates.get();
    m_updates_view->set_visible(false);
    m_updates_view->on_loading_state_changed = [this](bool loading, const std::string& msg) {
        set_status(msg, loading);
        if (m_btn_update_all) m_btn_update_all->set_enabled(!loading);
        if (m_group_btn) m_group_btn->set_enabled(!loading);
    };
    m_updates_view->on_updates_status_changed = [this](int update_count) {
        if (m_btn_update_all) {
            m_has_updates = (update_count > 0);
            if (m_group_btn && m_group_btn->current_item() == 2) {
                m_btn_update_all->set_visible(m_has_updates);
            }
        }
    };
    m_content_area->add_child(std::move(updates));

    this->when_application_load.connect([this, initial_search](auto&) {
        if (m_explore_view) {
            if (!initial_search.empty()) {
                m_explore_view->perform_search(initial_search);
            } else {
                m_explore_view->load_initial_data();
            }
        }
        if (m_updates_view) {
            m_updates_view->load_initial_data();
        }
        if (m_featured_view) {
            m_featured_view->load_initial_data();
        }
    });

    set_content(std::move(content));
}

void AppStoreWindow::setup_statusbar() {
    show_status_bar();
    auto sb = statusbar();
    
    auto lbl = std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.status.ready"));
    m_status_label = lbl.get();

    auto pbc = std::make_unique<horizon::Widget>();
    auto pb = std::make_unique<horizon::ProgressBar>();

    m_progress_bar = pb.get();
    m_progress_bar->set_indeterminate(true);
    m_progress_bar->set_visible(false);
    m_progress_bar->set_fixed_size(10); // height

    pbc->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    pbc->set_fixed_size(200); // width

    pbc->add_child(horizon::Spacer());
    pbc->add_child(std::move(pb));
    pbc->add_child(horizon::Spacer());

    sb->add_child(horizon::Spacer(10));
    sb->add_child(std::move(lbl));
    sb->add_child(std::move(pbc));
    sb->add_child(horizon::Spacer(10));
}

void AppStoreWindow::set_status(const std::string& message, bool loading) {
    if (m_status_label) m_status_label->set_text(message);
    if (m_progress_bar) m_progress_bar->set_visible(loading);
}

}
