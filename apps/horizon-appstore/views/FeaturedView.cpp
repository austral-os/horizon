#include "FeaturedView.hpp"
#include "horizon/Spacer.hpp"
#include <horizon/Application.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Image.hpp>
#include <horizon/Label.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/StarRating.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon::appstore
{

    enum class SectionLayoutMode
    {
        Stack,
        Flow
    };

    class SectionWidget : public horizon::Widget
    {
    public:
        SectionWidget(const std::string &title, SectionLayoutMode mode)
            : m_title_str(title), m_mode(mode)
        {
            set_position_type(horizon::FREE);

            auto lbl = std::make_unique<horizon::Label>(title);
            m_title_label = lbl.get();
            m_title_label->set_font_weight(horizon::FONT_WEIGHT_BOLD);
            m_title_label->set_font_size(18);
            m_title_label->set_text_color(horizon::Color(1.0f, 1.0f, 1.0f, 1.0f));
            m_title_label->set_position_type(horizon::FREE);
            add_child(std::move(lbl));
        }

        void calculate_layout() override
        {
            int current_y = 50; // header height + padding
            int current_x = 10;
            int max_row_height = 0;

            for (const auto &child : children())
            {
                if (child.get() == m_title_label)
                {
                    child->set_position(x() + 10, y() + 10);
                    child->set_size(width() - 20, 30);
                    continue;
                }
                if (m_mode == SectionLayoutMode::Stack)
                {
                    child->set_position(x() + 10, y() + current_y);
                    current_y += child->height() + 10;
                }
                else
                {
                    if (current_x + child->width() > width() - 10)
                    {
                        current_x = 10;
                        current_y += max_row_height + 10;
                        max_row_height = 0;
                    }
                    child->set_position(x() + current_x, y() + current_y);
                    max_row_height = std::max(max_row_height, child->height());
                    current_x += child->width() + 10;
                }
                child->calculate_layout();
            }

            if (m_mode == SectionLayoutMode::Flow && max_row_height > 0)
            {
                current_y += max_row_height + 10;
            }

            if (height() != current_y)
            {
                set_height(current_y);
            }
        }

        void draw(horizon::GraphicsContext &gc) override
        {
            gc.save();
            // Draw background
            gc.setColor(horizon::Color(0.12f, 0.12f, 0.12f, 1.0f));
            gc.clipRoundedRect(x(), y(), width(), height(), 8);
            gc.fillRect(x(), y(), width(), height());

            // Draw header (top rounded, bottom flat)
            gc.setColor(horizon::Color(0.08f, 0.08f, 0.08f, 1.0f));
            gc.fillRect(x(), y(), width(), 40);
            gc.restore();

            horizon::Widget::draw(gc);
        }

    private:
        std::string m_title_str;
        SectionLayoutMode m_mode;
        horizon::Label *m_title_label = nullptr;
    };

    class FeaturedBannerWidget : public horizon::Widget
    {
    public:
        FeaturedBannerWidget()
        {
            // No layout type, we position children manually

            auto cf = std::make_unique<horizon::CoverFlow<horizon::apt::FeaturedApp>>();
            m_coverflow = cf.get();
            m_coverflow->set_position_type(horizon::FREE);
            m_coverflow->set_item_size(256, 256); // Icon size for featured apps
            add_child(std::move(cf));

            auto lbl_title = std::make_unique<horizon::Label>();
            m_title = lbl_title.get();
            m_title->set_position_type(horizon::FREE);
            m_title->set_font_size(24);
            m_title->set_font_weight(horizon::FONT_WEIGHT_BOLD);
            m_title->set_text_color(horizon::Color(1.0f, 1.0f, 1.0f, 1.0f));
            m_title->set_cursor_type(horizon::CursorType::Pointer);
            add_child(std::move(lbl_title));

            auto lbl_desc = std::make_unique<horizon::Label>();
            m_description = lbl_desc.get();
            m_description->set_position_type(horizon::FREE);
            m_description->set_font_size(14);
            m_description->set_text_color(horizon::Color(0.8f, 0.8f, 0.8f, 1.0f));
            m_description->set_vertical_alignment(horizon::VerticalAlignment::Top);
            m_description->set_cursor_type(horizon::CursorType::Pointer);
            add_child(std::move(lbl_desc));

            auto rating = std::make_unique<horizon::StarRating>();
            m_rating = rating.get();
            m_rating->set_position_type(horizon::FREE);
            m_rating->set_readonly(true);
            add_child(std::move(rating));

            auto btn_prev = std::make_unique<horizon::ToolbarButton>("", "go-previous", 24);
            m_btn_prev = btn_prev.get();
            m_btn_prev->set_position_type(horizon::FREE);
            add_child(std::move(btn_prev));

            auto btn_next = std::make_unique<horizon::ToolbarButton>("", "go-next", 24);
            m_btn_next = btn_next.get();
            m_btn_next->set_position_type(horizon::FREE);
            add_child(std::move(btn_next));

            auto cat_sec = std::make_unique<SectionWidget>("Categorías", SectionLayoutMode::Stack);
            m_categories_section = cat_sec.get();
            add_child(std::move(cat_sec));

            auto top_sec =
                std::make_unique<SectionWidget>("Mejor Valoradas", SectionLayoutMode::Flow);
            m_top_rated_section = top_sec.get();
            add_child(std::move(top_sec));

            m_coverflow->when_index_changed.connect(
                [this](auto &)
                {
                    int idx = m_coverflow->selected_index();
                    const auto &data = m_coverflow->data();
                    if (idx >= 0 && idx < (int)data.size())
                    {
                        m_title->set_text(data[idx].name);
                        m_description->set_text(data[idx].description);
                        m_rating->set_rating(data[idx].avg_rating);
                    }
                });

            m_btn_prev->when_click.connect(
                [this](auto &)
                {
                    int idx = m_coverflow->selected_index();
                    const auto &data = m_coverflow->data();
                    if (!data.empty())
                    {
                        idx--;
                        if (idx < 0)
                            idx = data.size() - 1;
                        m_coverflow->set_selected_index(idx);
                    }
                });

            m_btn_next->when_click.connect(
                [this](auto &)
                {
                    int idx = m_coverflow->selected_index();
                    const auto &data = m_coverflow->data();
                    if (!data.empty())
                    {
                        idx++;
                        if (idx >= (int)data.size())
                            idx = 0;
                        m_coverflow->set_selected_index(idx);
                    }
                });

            auto click_handler = [this](horizon::MouseButtonEventContext &ev)
            {
                if (ev.button == 0x110)
                { // left click
                    int idx = m_coverflow->selected_index();
                    const auto &data = m_coverflow->data();
                    if (idx >= 0 && idx < (int)data.size() && on_app_clicked)
                    {
                        on_app_clicked(data[idx].package_name);
                    }
                }
            };
            m_title->when_mouse_press.connect(click_handler);
            m_description->when_mouse_press.connect(click_handler);
        }

        void calculate_layout() override
        {
            int w = width();
            if (w <= 0)
                w = 800; // fallback

            int h = 500;

            // Ocupar al menos el espacio del ScrollArea (opcional, como PackageDetailsWidget)
            if (parent())
            {
                h = std::max(h, parent()->height());
            }

            if (height() != h)
            {
                set_height(h);
            }

            int bx = x();
            int by = y();

            m_coverflow->set_position(bx, by);
            m_coverflow->set_size(w, 350);

            m_title->set_position(bx + 40, by + 370);
            m_title->set_size(w - 300, 30);

            m_rating->set_position(bx + w - 180, by + 375);
            m_rating->set_size(140, 24);

            m_description->set_position(bx + 40, by + 410);
            m_description->set_size(w - 200, 70);

            m_btn_prev->set_position(bx + w - 120, by + 440);
            m_btn_prev->set_size(40, 40);

            m_btn_next->set_position(bx + w - 60, by + 440);
            m_btn_next->set_size(40, 40);

            int sec_y = by + 500;
            m_categories_section->set_position(bx + 40, sec_y);
            m_categories_section->set_width(250);
            m_categories_section->calculate_layout();

            m_top_rated_section->set_position(bx + 310, sec_y);
            m_top_rated_section->set_width(w - 350);
            m_top_rated_section->calculate_layout();

            // Update FeaturedBannerWidget height based on sections
            int max_sec_h = std::max(m_categories_section->height(), m_top_rated_section->height());
            if (max_sec_h > 0)
            {
                h = std::max(h, 500 + max_sec_h + 40); // 40 is bottom padding
                if (height() != h)
                {
                    set_height(h);
                }
            }
        }

        void draw(horizon::GraphicsContext &gc) override
        {
            gc.setColor(horizon::Color(0.05f, 0.05f, 0.05f, 1.0f));
            gc.fillRect(x(), y(), width(), height());
            Widget::draw(gc);
        }

        horizon::CoverFlow<horizon::apt::FeaturedApp> *m_coverflow = nullptr;
        horizon::Label *m_title = nullptr;
        horizon::Label *m_description = nullptr;
        horizon::StarRating *m_rating = nullptr;
        horizon::ToolbarButton *m_btn_prev = nullptr;
        horizon::ToolbarButton *m_btn_next = nullptr;
        SectionWidget *m_categories_section = nullptr;
        SectionWidget *m_top_rated_section = nullptr;
        std::function<void(const std::string &)> on_app_clicked;

    public:
        SectionWidget *categories_section() const
        {
            return m_categories_section;
        }
        SectionWidget *top_rated_section() const
        {
            return m_top_rated_section;
        }
    };

    class LoadingWidget : public horizon::Widget
    {
    public:
        LoadingWidget()
        {
            set_position_type(horizon::FILL);

            auto container = std::make_unique<horizon::Widget>();
            container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);

            auto progress_wrapper = std::make_unique<horizon::Widget>();
            progress_wrapper->set_fixed_size(25);
            progress_wrapper->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);

            auto progress = std::make_unique<horizon::ProgressBar>();
            m_progress = progress.get();
            m_progress->set_indeterminate(true);

            progress_wrapper->add_child(Spacer());
            progress_wrapper->add_child(std::move(progress));
            progress_wrapper->add_child(Spacer());

            auto lbl =
                std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.featured.loading"));
            lbl->set_fixed_size(40);
            m_lbl = lbl.get();
            m_lbl->set_text_color(horizon::Color(1.0f, 1.0f, 1.0f, 1.0f));
            m_lbl->set_alignment(horizon::TextAlignment::Center);

            container->add_child(Spacer());
            container->add_child(std::move(progress_wrapper));
            container->add_child(std::move(lbl));
            container->add_child(Spacer());

            add_child(std::move(container));
        }

        void calculate_layout() override
        {
            int w = width();
            int h = height();
            if (m_progress)
            {
                m_progress->set_position((w - 200) / 2, (h - 20) / 2);
                m_progress->set_size(200, 20);
            }
            if (m_lbl)
            {
                m_lbl->set_position(0, (h - 20) / 2 + 30);
                m_lbl->set_size(w, 30);
            }
            Widget::calculate_layout();
        }

        void draw(horizon::GraphicsContext &gc) override
        {
            gc.setColor(horizon::Color(0.0f, 0.0f, 0.0f, 1.0f));
            gc.fillRect(x(), y(), width(), height());
            Widget::draw(gc);
        }

    private:
        horizon::ProgressBar *m_progress = nullptr;
        horizon::Label *m_lbl = nullptr;
    };

    FeaturedView::FeaturedView()
    {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::FILL);
        m_is_alive = std::make_shared<bool>(true);
        setup_ui();
    }

    FeaturedView::~FeaturedView()
    {
        *m_is_alive = false;
    }

    void FeaturedView::setup_ui()
    {
        auto scroll_area = std::make_unique<horizon::ScrollArea>();
        m_scroll_area = scroll_area.get();
        m_scroll_area->set_position_type(horizon::FILL);
        m_scroll_area->set_visible(false);

        auto banner = std::make_unique<FeaturedBannerWidget>();
        m_banner = banner.get();
        m_coverflow = m_banner->m_coverflow;

        m_banner->on_app_clicked = [this](const std::string &pkg_name)
        {
            AppClickedContext ctx{pkg_name};
            when_app_clicked.run(ctx);
        };

        m_coverflow->set_item_factory(
            [this](const horizon::apt::FeaturedApp &app, bool selected)
            {
                auto img = std::make_unique<horizon::Image>();
                img->set_mode(horizon::ImageMode::Fit);

                m_api_client.download_image_async(
                    app.icon_path,
                    [img_ptr = img.get(), alive = m_is_alive, this](std::optional<std::string> path)
                    {
                        if (path && application())
                        {
                            application()->post_task(
                                [img_ptr, p = *path, alive, this]()
                                {
                                    if (*alive)
                                    {
                                        bool found = false;
                                        if (m_coverflow)
                                        {
                                            for (const auto &child : m_coverflow->children())
                                            {
                                                if (child.get() == img_ptr)
                                                {
                                                    found = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (found)
                                        {
                                            img_ptr->set_path(p);
                                            m_coverflow->invalidate();
                                        }
                                    }
                                });
                        }
                    });

                return img;
            });

        m_scroll_area->set_content(std::move(banner));

        auto loading = std::make_unique<LoadingWidget>();
        m_loading_widget = loading.get();

        add_child(std::move(loading));
        add_child(std::move(scroll_area));
    }

    void FeaturedView::check_hide_loading()
    {
        if (m_featured_loaded && m_top_rated_loaded)
        {
            if (m_loading_widget)
            {
                m_loading_widget->set_visible(false);
            }
            if (m_scroll_area)
            {
                m_scroll_area->set_visible(true);
            }
            calculate_layout();
        }
    }

    void FeaturedView::reload_data()
    {
        clear_children();
        m_data_loaded = false;
        m_featured_loaded = false;
        m_top_rated_loaded = false;
        
        // Recrear la UI
        setup_ui();
        
        // Cargar los datos frescos
        load_initial_data();
    }

    void FeaturedView::load_initial_data()
    {
        if (m_data_loaded)
            return;
        m_data_loaded = true;

        // Categorías
        if (m_banner && m_banner->categories_section())
        {
            auto cat_sec = m_banner->categories_section();
            std::vector<std::pair<std::string, std::string>> categories = {
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
                {horizon::i18n().tr("appstore.category.other"), "applications-other"}};
            for (const auto &cat : categories)
            {
                auto item = std::make_unique<horizon::Widget>();
                item->set_size(230, 32);
                item->set_position_type(horizon::FREE);
                item->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
                item->set_spacing(10);
                item->set_cursor_type(horizon::CursorType::Pointer);

                auto icon = std::make_unique<horizon::Icon>();
                icon->set_icon_name(cat.second);
                icon->set_icon_size(24);
                icon->set_fixed_size(24);
                icon->set_cursor_type(horizon::CursorType::Pointer);
                item->add_child(std::move(icon));

                auto lbl = std::make_unique<horizon::Label>(cat.first);
                lbl->set_text_color(horizon::Color(0.8f, 0.8f, 0.8f, 1.0f));
                lbl->set_cursor_type(horizon::CursorType::Pointer);
                item->add_child(std::move(lbl));

                item->when_click.connect(
                    [this, cat_name = cat.first](auto &)
                    {
                        CategoryClickedContext ctx;
                        ctx.category_name = cat_name;
                        when_category_clicked.run(ctx);
                    });

                cat_sec->add_child(std::move(item));
            }
            cat_sec->calculate_layout();
            m_banner->calculate_layout();
        }

        // Aplicaciones mejor valoradas
        m_api_client.get_apps_async(
            std::nullopt, "rating", "desc", 15, 0, "es",
            [this, alive = m_is_alive](std::optional<std::vector<horizon::apt::AppInfo>> apps)
            {
                if (application())
                {
                    application()->post_task(
                        [this, alive, apps]()
                        {
                            if (!*alive)
                                return;
                            if (apps && m_banner && m_banner->top_rated_section())
                            {
                                auto apps_val = *apps;
                                auto top_sec = m_banner->top_rated_section();
                                for (size_t i = 0; i < apps_val.size() && i < 15; i++)
                                {
                                    const auto &app = apps_val[i];
                                    auto item = std::make_unique<horizon::Widget>();
                                    item->set_size(300, 80);
                                    item->set_position_type(horizon::FREE);
                                    item->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
                                    item->set_cursor_type(horizon::CursorType::Pointer);
                                    item->set_spacing(10);

                                    if (app.icon_path.empty())
                                    {
                                        auto icon = std::make_unique<horizon::Icon>();
                                        icon->set_icon_name("system-software-install");
                                        icon->set_icon_size(64);
                                        icon->set_fixed_size(64);
                                        icon->set_cursor_type(horizon::CursorType::Pointer);
                                        item->add_child(std::move(icon));
                                    }
                                    else
                                    {
                                        auto img = std::make_unique<horizon::Image>();
                                        img->set_mode(horizon::ImageMode::Fit);
                                        img->set_size(64, 64);
                                        img->set_fixed_size(64);
                                        img->set_cursor_type(horizon::CursorType::Pointer);

                                        m_api_client.download_image_async(
                                            app.icon_path,
                                            [img_ptr = img.get(), alive,
                                             this](std::optional<std::string> path)
                                            {
                                                if (path && application())
                                                {
                                                    application()->post_task(
                                                        [img_ptr, p = *path, alive]()
                                                        {
                                                            if (!*alive)
                                                                return;
                                                            img_ptr->set_path(p);
                                                            img_ptr->invalidate();
                                                        });
                                                }
                                            });
                                        item->add_child(std::move(img));
                                    }

                                    auto vpanel = std::make_unique<horizon::Widget>();
                                    vpanel->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
                                    vpanel->set_spacing(5);
                                    vpanel->set_cursor_type(horizon::CursorType::Pointer);

                                    auto lbl_name = std::make_unique<horizon::Label>(app.name);
                                    lbl_name->set_font_weight(horizon::FONT_WEIGHT_BOLD);
                                    lbl_name->set_text_color(
                                        horizon::Color(1.0f, 1.0f, 1.0f, 1.0f));
                                    lbl_name->set_fixed_size(20);
                                    lbl_name->set_cursor_type(horizon::CursorType::Pointer);
                                    vpanel->add_child(std::move(lbl_name));

                                    auto rating = std::make_unique<horizon::StarRating>();
                                    rating->set_star_size(14);
                                    rating->set_size(90, 14);
                                    rating->set_fixed_size(14);
                                    rating->set_readonly(true);
                                    rating->set_rating(app.avg_rating);
                                    vpanel->add_child(std::move(rating));

                                    item->add_child(std::move(vpanel));

                                    item->when_click.connect(
                                        [this, pkg = app.package_name](auto &)
                                        {
                                            AppClickedContext ctx;
                                            ctx.package_name = pkg;
                                            when_app_clicked.run(ctx);
                                        });

                                    top_sec->add_child(std::move(item));
                                }
                                top_sec->calculate_layout();
                                m_banner->calculate_layout();
                            }
                            m_top_rated_loaded = true;
                            check_hide_loading();
                        });
                }
            });

        m_api_client.get_featured_apps_async(
            "es",
            [this, alive = m_is_alive](std::optional<std::vector<horizon::apt::FeaturedApp>> apps)
            {
                if (application())
                {
                    application()->post_task(
                        [this, alive, apps]()
                        {
                            if (!*alive)
                                return;
                            if (apps && m_coverflow)
                            {
                                auto apps_val = *apps;
                                m_coverflow->set_data(apps_val);
                                // trigger selection manually to populate title/description
                                if (!apps_val.empty())
                                {
                                    horizon::EventContext ctx;
                                    m_coverflow->when_index_changed.run(ctx);
                                }
                            }
                            m_featured_loaded = true;
                            check_hide_loading();
                        });
                }
            });
    }

} // namespace horizon::appstore
