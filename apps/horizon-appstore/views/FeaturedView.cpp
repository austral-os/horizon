#include "FeaturedView.hpp"
#include <horizon/Label.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Image.hpp>
#include <horizon/Application.hpp>

namespace horizon::appstore {

class FeaturedBannerWidget : public horizon::Widget {
public:
    FeaturedBannerWidget() {
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
        
        auto btn_prev = std::make_unique<horizon::ToolbarButton>("", "go-previous", 24);
        m_btn_prev = btn_prev.get();
        m_btn_prev->set_position_type(horizon::FREE);
        add_child(std::move(btn_prev));
        
        auto btn_next = std::make_unique<horizon::ToolbarButton>("", "go-next", 24);
        m_btn_next = btn_next.get();
        m_btn_next->set_position_type(horizon::FREE);
        add_child(std::move(btn_next));
        
        m_coverflow->when_index_changed.connect([this](auto&) {
            int idx = m_coverflow->selected_index();
            const auto& data = m_coverflow->data();
            if (idx >= 0 && idx < (int)data.size()) {
                m_title->set_text(data[idx].name);
                m_description->set_text(data[idx].description);
            }
        });
        
        m_btn_prev->when_click.connect([this](auto&) {
            int idx = m_coverflow->selected_index();
            const auto& data = m_coverflow->data();
            if (!data.empty()) {
                idx--;
                if (idx < 0) idx = data.size() - 1;
                m_coverflow->set_selected_index(idx);
            }
        });
        
        m_btn_next->when_click.connect([this](auto&) {
            int idx = m_coverflow->selected_index();
            const auto& data = m_coverflow->data();
            if (!data.empty()) {
                idx++;
                if (idx >= (int)data.size()) idx = 0;
                m_coverflow->set_selected_index(idx);
            }
        });
        
        auto click_handler = [this](horizon::MouseButtonEventContext& ev) {
            if (ev.button == 0x110) { // left click
                int idx = m_coverflow->selected_index();
                const auto& data = m_coverflow->data();
                if (idx >= 0 && idx < (int)data.size() && on_app_clicked) {
                    on_app_clicked(data[idx].package_name);
                }
            }
        };
        m_title->when_mouse_press.connect(click_handler);
        m_description->when_mouse_press.connect(click_handler);
    }

    void calculate_layout() override {
        int w = width();
        if (w <= 0) w = 800; // fallback
        
        int h = 500;
        
        // Ocupar al menos el espacio del ScrollArea (opcional, como PackageDetailsWidget)
        if (parent()) {
            h = std::max(h, parent()->height());
        }
        
        if (height() != h) {
            set_height(h);
        }
        
        int bx = x();
        int by = y();
        
        m_coverflow->set_position(bx, by);
        m_coverflow->set_size(w, 350);
        
        m_title->set_position(bx + 40, by + 370);
        m_title->set_size(w - 200, 30);
        
        m_description->set_position(bx + 40, by + 410);
        m_description->set_size(w - 200, 70);
        
        m_btn_prev->set_position(bx + w - 120, by + 440);
        m_btn_prev->set_size(40, 40);
        
        m_btn_next->set_position(bx + w - 60, by + 440);
        m_btn_next->set_size(40, 40);
    }
    
    void draw(horizon::GraphicsContext& gc) override {
        gc.setColor(horizon::Color(0.05f, 0.05f, 0.05f, 1.0f));
        gc.fillRect(x(), y(), width(), height());
        Widget::draw(gc);
    }

    horizon::CoverFlow<horizon::apt::FeaturedApp>* m_coverflow = nullptr;
    horizon::Label* m_title = nullptr;
    horizon::Label* m_description = nullptr;
    horizon::ToolbarButton* m_btn_prev = nullptr;
    horizon::ToolbarButton* m_btn_next = nullptr;
    std::function<void(const std::string&)> on_app_clicked;
};

FeaturedView::FeaturedView() {
    set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    set_position_type(horizon::FILL);
    m_is_alive = std::make_shared<bool>(true);
    setup_ui();
}

FeaturedView::~FeaturedView() {
    *m_is_alive = false;
}

void FeaturedView::setup_ui() {
    auto scroll_area = std::make_unique<horizon::ScrollArea>();
    m_scroll_area = scroll_area.get();
    m_scroll_area->set_position_type(horizon::FILL);
    
    auto banner = std::make_unique<FeaturedBannerWidget>();
    m_banner = banner.get();
    m_coverflow = m_banner->m_coverflow;
    
    m_banner->on_app_clicked = [this](const std::string& pkg_name) {
        AppClickedContext ctx{pkg_name};
        when_app_clicked.run(ctx);
    };
    
    m_coverflow->set_item_factory([this](const horizon::apt::FeaturedApp& app, bool selected) {
        auto img = std::make_unique<horizon::Image>();
        img->set_mode(horizon::ImageMode::Fit);
        
        m_api_client.download_image_async(app.icon_path, [img_ptr = img.get(), alive = m_is_alive, this](std::optional<std::string> path) {
            if (path && application()) {
                application()->post_task([img_ptr, p = *path, alive, this]() {
                    if (*alive) {
                        bool found = false;
                        if (m_coverflow) {
                            for (const auto& child : m_coverflow->children()) {
                                if (child.get() == img_ptr) { found = true; break; }
                            }
                        }
                        if (found) {
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
    add_child(std::move(scroll_area));
}

void FeaturedView::load_initial_data() {
    if (m_data_loaded) return;
    m_data_loaded = true;
    m_api_client.get_featured_apps_async("es", [this, alive = m_is_alive](std::optional<std::vector<horizon::apt::FeaturedApp>> apps) {
        if (apps && application()) {
            application()->post_task([this, alive, apps_val = *apps]() {
                if (!*alive) return;
                if (m_coverflow) {
                    m_coverflow->set_data(apps_val);
                    // trigger selection manually to populate title/description
                    if (!apps_val.empty()) {
                        horizon::EventContext ctx;
                        m_coverflow->when_index_changed.run(ctx);
                    }
                }
            });
        }
    });
}

}
