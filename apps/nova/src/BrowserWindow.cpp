#include "BrowserWindow.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Spacer.hpp"
#include <memory>

namespace horizon
{
    namespace nova
    {

        BrowserWindow::BrowserWindow() : ApplicationWindow("Nova Web Browser")
        {
            set_size(1024, 768);
            setup_ui();
        }

        void BrowserWindow::setup_ui()
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

            m_tabs->when_add_tab_clicked.connect(
                [this](EventContext &)
                { this->create_new_tab("https://www.youtube.com/watch?v=nDnecvEbaQ8"); });

            m_tabs->when_tab_selected.connect(
                [this](int index)
                {
                    auto *web_view = dynamic_cast<web::WebWidget *>(m_tabs->current_tab_body());
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
                    auto *web_view = dynamic_cast<web::WebWidget *>(m_tabs->current_tab_body());
                    if (!web_view)
                        return;
                    if (ctx.index == 0)
                        web_view->go_back();
                    else
                        web_view->go_forward();
                });

            m_toolbar->when_home_clicked.connect(
                [this](HomeButtonClickEvent &)
                { this->navigate_to_url("https://www.youtube.com/watch?v=nDnecvEbaQ8"); });

            m_toolbar->when_search_submitted.connect([this](SearchChangedEvent &ctx)
                                                     { this->navigate_to_url(ctx.query); });

            m_toolbar->when_bookmark_clicked.connect([this](BookmarkButtonClickEvent &)
                                                     { LOG_INFO << "[NOVA] Bookmarks clicked"; });

            m_toolbar->when_options_clicked.connect([this](OptionsButtonClickEvent &)
                                                    { LOG_INFO << "[NOVA] Options clicked"; });

            // Initial tab
            create_new_tab("https://www.youtube.com/watch?v=nDnecvEbaQ8");

            set_content(std::move(tabs));
        }

        void BrowserWindow::create_new_tab(const std::string &url)
        {
            LOG_INFO << "[NOVA] Creating new tab for URL: " << url;
            auto web_view = std::make_unique<web::WebWidget>();
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
            ptr->set_focus(true);

            ptr->when_title_changed.connect([this, ptr, index](const std::string &title)
                                            { m_tabs->set_tab_title(index, title); });

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

            ptr->when_fullscreen_changed.connect(
                [this](bool fullscreen)
                {
                    if (this->application())
                    {
                        if (fullscreen)
                        {
                            this->set_immersive_mode(true);
                            this->application()->fullscreen();
                            if (m_tabs)
                                m_tabs->show_header(false);
                            this->hide_status_bar();
                        }
                        else
                        {
                            this->set_immersive_mode(false);
                            this->application()->unfullscreen();
                            if (m_tabs)
                                m_tabs->show_header(true);
                            this->show_status_bar();
                        }
                    }
                });

            ptr->load_url(url);
        }

        void BrowserWindow::navigate_to_url(const std::string &input_url)
        {
            auto *web_view = dynamic_cast<web::WebWidget *>(m_tabs->current_tab_body());
            if (web_view)
            {
                std::string url = input_url;
                if (url.empty())
                    return;

                // Basic URL normalization
                if (url.find("://") == std::string::npos)
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

                LOG_INFO << "[NOVA] Navigating in current tab to: " << url;
                if (web_view)
                {
                    web_view->load_url(url);
                }
                else
                {
                    LOG_ERROR << "[NOVA] Cannot navigate: current_tab_body is NULL";
                }
            }
        }

    } // namespace nova
} // namespace horizon
