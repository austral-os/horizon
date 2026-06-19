#include <horizon/RibbonButton.hpp>
#include <horizon/Vault.hpp>

namespace horizon
{

    RibbonButton::RibbonButton()
    {
        this->set_focusable(true);

        m_background = std::make_unique<SolidObject>();
        m_background->set_corner_radius({4, 4, 4, 4});

        m_icon = std::make_unique<Icon>();
        m_icon->set_horizontal_alignment(TextAlignment::Center);

        m_label = std::make_unique<Label>();
        m_label->set_alignment(TextAlignment::Center);
        m_label->set_font_size(12);

        this->when_mouse_enter.connect([this](EventContext &) {
            m_hovered = true;
            this->invalidate();
        });
        this->when_mouse_leave.connect([this](EventContext &) {
            m_hovered = false;
            this->invalidate();
        });
        this->when_mouse_press.connect([this](MouseButtonEventContext &) { this->invalidate(); });
        this->when_mouse_release.connect([this](MouseButtonEventContext &) { this->invalidate(); });

        update_icon_size();
    }

    void RibbonButton::set_icon(const std::string &icon_name)
    {
        m_icon->set_icon_name(icon_name);
        this->invalidate();
    }

    void RibbonButton::set_text(const std::string &text)
    {
        m_label->set_text(text);
        this->invalidate();
    }

    void RibbonButton::set_text_position(RibbonButtonTextPosition position)
    {
        m_text_position = position;
        this->invalidate();
    }

    void RibbonButton::set_button_size(RibbonButtonSize size)
    {
        m_button_size = size;
        update_icon_size();
        this->invalidate();
    }

    void RibbonButton::set_active(bool active)
    {
        if (m_active != active) {
            m_active = active;
            this->invalidate();
        }
    }

    bool RibbonButton::is_active() const
    {
        return m_active;
    }

    void RibbonButton::set_font_size(int size)
    {
        m_label->set_font_size(size);
        this->invalidate();
    }

    int RibbonButton::font_size() const
    {
        return m_label->font_size();
    }

    void RibbonButton::set_font_weight(FontWeight weight)
    {
        m_label->set_font_weight(weight);
        this->invalidate();
    }

    FontWeight RibbonButton::font_weight() const
    {
        return m_label->font_weight();
    }

    void RibbonButton::update_icon_size()
    {
        int size = 24;
        switch (m_button_size)
        {
        case RibbonButtonSize::XLarge:
            size = 48;
            break;
        case RibbonButtonSize::Large:
            size = 32;
            break;
        case RibbonButtonSize::Normal:
            size = 24;
            break;
        case RibbonButtonSize::Small:
            size = 16;
            break;
        case RibbonButtonSize::XSmall:
            size = 12;
            break;
        }
        m_icon->set_icon_size(size);
    }

    int RibbonButton::preferred_width() const
    {
        int padding = 12; // total horizontal padding
        int icon_w = m_icon->preferred_width();
        int label_w = m_label->text().empty() ? 0 : m_label->preferred_width();
        int spacing = 6;
        int vault_w = (this->vault() != nullptr) ? 12 : 0;

        if (m_label->text().empty())
        {
            return padding + icon_w + (vault_w ? vault_w + spacing : 0);
        }

        if (m_text_position == RibbonButtonTextPosition::BelowIcon)
        {
            return padding + std::max(icon_w, label_w) + (vault_w ? vault_w + spacing : 0);
        }
        else // RightOfIcon
        {
            return padding + icon_w + spacing + label_w + (vault_w ? spacing + vault_w : 0);
        }
    }

    int RibbonButton::preferred_height() const
    {
        int padding = 12; // total vertical padding
        int icon_h = m_icon->preferred_height();
        int label_h = m_label->text().empty() ? 0 : m_label->preferred_height();
        int spacing = 4;

        if (m_label->text().empty())
        {
            return padding + icon_h;
        }

        if (m_text_position == RibbonButtonTextPosition::BelowIcon)
        {
            return padding + icon_h + spacing + label_h;
        }
        else // RightOfIcon
        {
            return padding + std::max(icon_h, label_h);
        }
    }

    int RibbonButton::preferred_height(int width) const
    {
        return preferred_height();
    }

    void RibbonButton::calculate_layout()
    {
        Widget::calculate_layout();

        m_background->set_position(m_x, m_y);
        m_background->set_size(m_width, m_height);

        int icon_w = m_icon->preferred_width();
        int icon_h = m_icon->preferred_height();
        int label_w = m_label->text().empty() ? 0 : m_label->preferred_width();
        int label_h = m_label->text().empty() ? 0 : m_label->preferred_height();
        int spacing = 4;

        if (m_label->text().empty())
        {
            // Center icon both horizontally and vertically
            int ix = m_start_draw_x + (m_available_draw_width - icon_w) / 2;
            int iy = m_start_draw_y + (m_available_draw_height - icon_h) / 2;
            
            // Adjust if vault
            if (this->vault() != nullptr) {
                ix -= 6; 
            }
            
            m_icon->set_position(ix, iy);
            m_icon->set_size(icon_w, icon_h);
        }
        else if (m_text_position == RibbonButtonTextPosition::BelowIcon)
        {
            int total_h = icon_h + spacing + label_h;
            int start_y = m_start_draw_y + (m_available_draw_height - total_h) / 2;

            int ix = m_start_draw_x + (m_available_draw_width - icon_w) / 2;
            
            // Adjust if vault
            if (this->vault() != nullptr) {
                ix -= 6;
            }

            m_icon->set_position(ix, start_y);
            m_icon->set_size(icon_w, icon_h);

            int lx = m_start_draw_x + (m_available_draw_width - label_w) / 2;
            if (this->vault() != nullptr) {
                lx -= 6;
            }

            m_label->set_position(lx, start_y + icon_h + spacing);
            m_label->set_size(label_w, label_h);
        }
        else // RightOfIcon
        {
            int start_x = m_start_draw_x + 6; // Left padding
            int ix = start_x;
            int iy = m_start_draw_y + (m_available_draw_height - icon_h) / 2;

            m_icon->set_position(ix, iy);
            m_icon->set_size(icon_w, icon_h);

            int lx = ix + icon_w + spacing;
            int ly = m_start_draw_y + (m_available_draw_height - label_h) / 2;

            m_label->set_position(lx, ly);
            m_label->set_size(label_w, label_h);
        }

        m_icon->calculate_layout();
        m_label->calculate_layout();
        
        if (this->vault() != nullptr) {
             this->vault()->set_arrow_position(m_width / 2, m_height);
        }
    }

    void RibbonButton::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        m_background->set_application_recursive(app);
        m_icon->set_application_recursive(app);
        m_label->set_application_recursive(app);
    }

    void RibbonButton::draw(GraphicsContext &ctx)
    {
        Widget::draw(ctx);

        auto state = get_draw_state();
        if (m_hovered || m_active || state == WidgetDrawState::PRESSED)
        {
            auto *tm = theme_manager();
            Color bg_color = tm->get_color("button_bg_hovered");
            
            if (m_active || state == WidgetDrawState::PRESSED) {
                bg_color = tm->get_color("button_bg_active");
            }
            
            m_background->set_background_color(bg_color);
            m_background->set_application_recursive(application());
            m_background->set_position(m_start_draw_x, m_start_draw_y);
            m_background->set_size(m_available_draw_width, m_available_draw_height);
            m_background->calculate_layout();
            m_background->draw(ctx);
        }

        m_icon->render(ctx, m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height, true);

        if (!m_label->text().empty())
        {
            m_label->render(ctx, m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height, true);
        }

        if (this->vault() != nullptr)
        {
            // Draw a small arrow indicating the vault
            auto *tm = theme_manager();
            Color v_color = m_label->text_color();
            if (v_color.a < 0.0f) {
                v_color = tm->get_color("window_fg");
            }
            if (!is_enabled()) {
                v_color.a *= 0.4f;
            }
            ctx.setColor(v_color);

            int arrow_size = 4;
            int ax = 0;
            int ay = 0;

            if (m_label->text().empty())
            {
                ax = m_icon->x() + m_icon->width() + 6;
                ay = m_start_draw_y + m_available_draw_height / 2 - arrow_size / 2;
            }
            else if (m_text_position == RibbonButtonTextPosition::BelowIcon)
            {
                int text_right = m_label->x() + m_label->width();
                int icon_right = m_icon->x() + m_icon->width();
                ax = std::max(text_right, icon_right) + 6;
                ay = m_start_draw_y + m_available_draw_height / 2 - arrow_size / 2;
            }
            else // RightOfIcon
            {
                ax = m_start_draw_x + m_available_draw_width - arrow_size * 2 - 6;
                ay = m_start_draw_y + m_available_draw_height / 2 - arrow_size / 2;
            }

            std::vector<PolygonPoint> points = {
                {ax, ay},
                {ax + arrow_size * 2, ay},
                {ax + arrow_size, ay + arrow_size}
            };
            ctx.fillPolygon(points);
        }
    }

} // namespace horizon
