#include "BrowserWindow.hpp"
#include "horizon/GroupButton.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/WaylandWindow.hpp"
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <pwd.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace horizon
{
    namespace nova
    {

        const std::string DEFAULT_URL = "about:blank";

        BrowserWindow::BrowserWindow(const std::string &initial_url)
            : ApplicationWindow("Nova Web Browser")
        {
            set_size(1024, 768);

            // 1. Determine config path
            const char *home = getenv("HOME");
            if (home)
            {
                m_config_path = std::string(home) + "/.config/horizon/nova.json";
            }
            else
            {
                m_config_path = "nova.json";
            }

            load_preferences();

            setup_ui(initial_url);
        }

        void BrowserWindow::setup_ui(const std::string &initial_url)
        {
            // 1. Nova Toolbar
            auto nova_toolbar = std::make_unique<NovaToolbar>();
            m_toolbar = nova_toolbar.get();
            toolbar()->add_toolbar_widget(std::move(nova_toolbar));

            // 2. Status Bar
            show_status_bar();
            auto *sb = statusbar();

            auto status_lbl = std::make_unique<horizon::Label>("Ready");
            m_status_label = status_lbl.get();

            auto pb_container = std::make_unique<horizon::Widget>();
            pb_container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
            pb_container->set_fixed_size(200);

            auto pb = std::make_unique<horizon::ProgressBar>();
            m_progress_bar = pb.get();
            m_progress_bar->set_visible(false);
            m_progress_bar->set_fixed_size(10); // Height

            pb_container->add_child(horizon::Spacer());
            pb_container->add_child(std::move(pb));
            pb_container->add_child(horizon::Spacer());

            sb->add_child(horizon::Spacer(10));
            sb->add_child(std::move(status_lbl));
            sb->add_child(horizon::Spacer()); // Push to right
            sb->add_child(std::move(pb_container));
            sb->add_child(horizon::Spacer(10));

            // 3. Tab Collection
            auto tabs = std::make_unique<TabCollection>();
            m_tabs = tabs.get();
            m_tabs->set_smart_header(true);
            m_tabs->set_closable_tabs(true);

            m_tabs->when_add_tab_clicked.connect([this](EventContext &)
                                                 { this->create_new_tab(DEFAULT_URL); });

            m_tabs->when_tab_close_requested.connect(
                [this](int index)
                { application()->post_task([this, index]() { m_tabs->remove_tab(index); }); });

            m_tabs->when_items_changed.connect([this](int count)
                                               { m_toolbar->show_add_tab_button(count == 1); });

            m_toolbar->add_tab_button()->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &) { this->create_new_tab(DEFAULT_URL); });

            m_tabs->when_tab_selected.connect(
                [this](int index)
                {
                    auto *web_view = dynamic_cast<web::WebView *>(m_tabs->current_tab_body());
                    if (web_view)
                    {
                        m_toolbar->set_url(web_view->get_url());
                    }
                });

            // --- Connections ---

            // Toolbar -> Current Web View
            m_toolbar->when_navigation_clicked.connect(
                [this](NavigationButtonClickEvent &ctx)
                {
                    auto *web_view = dynamic_cast<web::WebView *>(m_tabs->current_tab_body());
                    if (!web_view)
                        return;
                    if (ctx.index == 0)
                        web_view->go_back();
                    else
                        web_view->go_forward();
                });

            m_toolbar->when_home_clicked.connect([this](HomeButtonClickEvent &)
                                                 { this->navigate_to_url(DEFAULT_URL); });

            m_toolbar->when_search_submitted.connect([this](SearchChangedEvent &ctx)
                                                     { this->navigate_to_url(ctx.query); });

            m_toolbar->when_bookmark_clicked.connect([this](BookmarkButtonClickEvent &)
                                                     { LOG_INFO << "[NOVA] Bookmarks clicked"; });

            m_toolbar->when_options_clicked.connect(
                [this](OptionsButtonClickEvent &)
                {
                    if (this->application())
                        this->application()->show_preferences();
                });

            // Shortcuts
            when_key_press.connect(
                [this](KeyEventContext &ctx)
                {
                    if ((ctx.modifiers & WaylandWindow::Modifier::CTRL) &&
                        (ctx.keysym == 0x6c || ctx.keysym == 0x4c)) // 'l' or 'L'
                    {
                        if (m_toolbar)
                        {
                            m_toolbar->focus_address_bar();
                            ctx.stop_propagation = true;
                        }
                    }
                });

            // Initial tab
            std::string start_url = initial_url;
            if (start_url.empty())
            {
                start_url = m_homepage.empty() ? DEFAULT_URL : m_homepage;
            }
            create_new_tab(start_url);

            set_content(std::move(tabs));
        }

        void BrowserWindow::create_new_tab(const std::string &url)
        {
            LOG_INFO << "[NOVA] Creating new tab for URL: " << url;
            auto web_view = std::make_unique<web::WebView>();
            auto *ptr = web_view.get();

            // Connect signals for the new web view
            ptr->when_url_changed.connect(
                [this, ptr](const std::string &url)
                {
                    if (m_tabs->current_tab_body() == (horizon::Widget *)ptr)
                    {
                        m_toolbar->set_url(url);
                    }
                });

            int index =
                m_tabs->add_tab("New Tab", std::unique_ptr<horizon::Widget>(web_view.release()));
            m_tabs->set_current_tab(index);
            ptr->set_focus(true);

            ptr->when_title_changed.connect([this, ptr](const std::string &title) {
                // Find the current index of this tab as it might have changed
                for (size_t i = 0; i < m_tabs->tab_count(); ++i) {
                    if (m_tabs->tab_body(i) == (horizon::Widget*)ptr) {
                        m_tabs->set_tab_title(i, title);
                        break;
                    }
                }
            });

            ptr->when_loading_changed.connect(
                [this, ptr](bool loading)
                {
                    if (m_tabs->current_tab_body() == (horizon::Widget *)ptr)
                    {
                        if (m_status_label)
                            m_status_label->set_text(loading ? "Loading..." : "Done");
                        if (m_progress_bar)
                            m_progress_bar->set_visible(loading);
                    }
                });

            ptr->when_progress_changed.connect(
                [this, ptr](double progress)
                {
                    if (m_tabs->current_tab_body() == (horizon::Widget *)ptr)
                    {
                        if (m_progress_bar)
                            m_progress_bar->set_progress((float)progress);
                    }
                });

            ptr->when_enter_fullscreen.connect([this](FullscreenEventContext &)
                                               { this->set_immersive_mode(true); });

            ptr->when_leave_fullscreen.connect([this](FullscreenEventContext &)
                                               { this->set_immersive_mode(false); });

            ptr->load_url(normalize_url(url));
        }

        std::string BrowserWindow::normalize_url(const std::string &input_url)
        {
            std::string url = input_url;
            if (url.empty())
                return "about:blank";

            // Basic URL normalization
            if (url.find("://") == std::string::npos && url.find("about:") != 0)
            {
                if (url.find(".") != std::string::npos && url.find(" ") == std::string::npos)
                {
                    url = "https://" + url;
                }
                else
                {
                    url = "https://www.google.com/search?q=" + url;
                }
            }
            return url;
        }

        void BrowserWindow::navigate_to_url(const std::string &input_url)
        {
            auto *web_view = dynamic_cast<web::WebView *>(m_tabs->current_tab_body());
            if (web_view)
            {
                std::string url = normalize_url(input_url);
                LOG_INFO << "[NOVA] Navigating in current tab to: " << url;
                web_view->load_url(url);
            }
        }

        void BrowserWindow::load_preferences()
        {
            std::ifstream f(m_config_path);
            if (!f.is_open())
                return;

            try
            {
                nlohmann::json j;
                f >> j;

                if (j.contains("general"))
                {
                    auto general = j["general"];
                    if (general.contains("homepage"))
                    {
                        m_homepage = general["homepage"].get<std::string>();
                    }
                    if (general.contains("use_gpu"))
                    {
                        bool use_gpu = general["use_gpu"].get<bool>();
                        web::WebView::set_gpu_enabled(use_gpu);
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "[NOVA] Error loading preferences: " << e.what();
            }
        }

    } // namespace nova
} // namespace horizon
