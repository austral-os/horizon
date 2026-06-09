#include "AppStoreWindow.hpp"
#include <horizon/Toolbar.hpp>
#include <horizon/Statusbar.hpp>
#include <horizon/Spacer.hpp>

namespace horizon::appstore {

AppStoreWindow::AppStoreWindow() : ApplicationWindow("Horizon AppStore") {
    set_size(1000, 700);
    
    m_apt_manager = std::make_unique<horizon::apt::AptManager>();
    m_apt_manager->initialize();

    setup_content();
    setup_toolbar();
    setup_statusbar();
}

void AppStoreWindow::setup_toolbar() {
    auto tb = toolbar();
    
    auto btn_action = std::make_unique<horizon::ToolbarButton>("Instalar", "system-software-install");
    m_btn_action = btn_action.get();
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
    tb->add_toolbar_widget(std::move(btn_action));
    
    tb->add_toolbar_widget(horizon::Spacer());

    auto group_btn = std::make_unique<horizon::ToggleGroupButton>();
    m_group_btn = group_btn.get();
    m_group_btn->add_item("Destacados");
    m_group_btn->add_item("Explorar");
    m_group_btn->add_item("Actualizaciones");
    m_group_btn->set_fixed_size(400);
    m_group_btn->set_current_item(0);

    m_group_btn->when_button_clicked.connect([this](horizon::GroupButtonClickEvent& ctx) {
        bool is_explore = (ctx.button_text == "Explorar");
        if (m_featured_view) m_featured_view->set_visible(ctx.button_text == "Destacados");
        if (m_explore_view) m_explore_view->set_visible(is_explore);
        if (m_updates_view) m_updates_view->set_visible(ctx.button_text == "Actualizaciones");
        
        if (m_btn_action) {
            if (is_explore && m_explore_view && m_explore_view->selected_package()) {
                m_btn_action->set_visible(true);
            } else {
                m_btn_action->set_visible(false);
            }
        }
    });

    tb->add_toolbar_widget(std::move(group_btn));
    tb->add_toolbar_widget(horizon::Spacer());

    auto search_box = std::make_unique<horizon::SearchBox>();
    m_search_box = search_box.get();
    m_search_box->set_placeholder("Buscar...");
    m_search_box->when_text_changed.connect([this](horizon::KeyEventContext&) {
        static size_t debounce_timer = 0;
        if (debounce_timer) application()->stop_timer(debounce_timer);
        debounce_timer = application()->add_timer(300, [this]() {
            if (m_explore_view) {
                m_explore_view->perform_search(m_search_box->text());
                // Switch to explore view if we are searching
                m_group_btn->set_current_item(1);
                if (m_featured_view) m_featured_view->set_visible(false);
                if (m_updates_view) m_updates_view->set_visible(false);
                m_explore_view->set_visible(true);
            }
        });
    });
    
    auto search_wrapper = std::make_unique<horizon::Widget>();
    search_wrapper->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    search_wrapper->set_fixed_size(250);
    search_wrapper->add_child(std::move(search_box));
    
    tb->add_toolbar_widget(std::move(search_wrapper));
    tb->add_toolbar_widget(horizon::Spacer(10));
}

void AppStoreWindow::setup_content() {
    auto content = std::make_unique<horizon::Widget>();
    m_content_area = content.get();
    m_content_area->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    
    auto featured = std::make_unique<FeaturedView>();
    m_featured_view = featured.get();
    m_content_area->add_child(std::move(featured));
    
    auto explore = std::make_unique<ExploreView>(m_apt_manager.get());
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
                m_btn_action->set_title("Desinstalar");
                m_btn_action->set_icon_name("edit-delete");
            } else {
                m_btn_action->set_title("Instalar");
                m_btn_action->set_icon_name("system-software-install");
            }
        } else {
            m_btn_action->set_visible(false);
        }
    };
    
    m_content_area->add_child(std::move(explore));
    
    this->when_application_load.connect([this](auto&) {
        if (m_explore_view) {
            m_explore_view->load_initial_data();
        }
    });
    
    auto updates = std::make_unique<UpdatesView>();
    m_updates_view = updates.get();
    m_updates_view->set_visible(false);
    m_content_area->add_child(std::move(updates));

    set_content(std::move(content));
}

void AppStoreWindow::setup_statusbar() {
    show_status_bar();
    auto sb = statusbar();
    
    auto lbl = std::make_unique<horizon::Label>("Listo");
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
