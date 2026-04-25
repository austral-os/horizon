#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/ThemeManager.hpp>
#include <linux/input-event-codes.h>

namespace horizon
{

    MenuItem::MenuItem() : Widget(), m_id("")
    {
        auto content = std::make_unique<Label>();
        m_content = content.get();
        m_content->set_position_type(FREE);
        static_cast<Label *>(m_content)->set_vertical_alignment(VerticalAlignment::Middle);
        add_child(std::move(content));

        auto shortcut = std::make_unique<Label>();
        m_shortcut_label = shortcut.get();
        m_shortcut_label->set_position_type(FREE);
        m_shortcut_label->set_vertical_alignment(VerticalAlignment::Middle);
        m_shortcut_label->set_alignment(TextAlignment::Right);
        add_child(std::move(shortcut));

        set_size(200, 24); // Default menu item size

        when_mouse_enter.connect(
            [this](EventContext &)
            {
                set_selected(true);
                if (auto *p = dynamic_cast<Menu *>(parent()))
                {
                    p->set_active_submenu(m_submenu.get());
                }
            });

        when_mouse_leave.connect([this](EventContext &) { set_selected(false); });
        
        when_mouse_press.connect([this](MouseButtonEventContext &ev) {
            if (m_has_submenu) {
                ev.stop_propagation = true;
            }
        });

        when_mouse_release.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (!is_enabled() || ev.button != BTN_LEFT)
                    return;

                LOG_INFO << "[MENUITEM] Triggering action for: " << text();

                // Trigger click signal for external listeners (like ArkFM)
                when_click.run(ev);

                auto app = application();
                if (app && m_emit_signal_manager)
                {
                    std::string signal_name = m_id.empty() ? text() : m_id;
                    app->signal_manager.emit(signal_name, this);
                }
            });
    }

    MenuItem::MenuItem(const std::string &text) : MenuItem()
    {
        set_text(text);
    }

    void MenuItem::set_text(const std::string &text)
    {
        if (auto *label = dynamic_cast<Label *>(m_content))
        {
            label->set_text(text);
        }
    }

    const std::string &MenuItem::text() const
    {
        if (auto *label = dynamic_cast<Label *>(m_content))
        {
            return label->text();
        }
        static std::string empty;
        return empty;
    }

    void MenuItem::set_shortcut(const std::string &shortcut)
    {
        m_shortcut_text = shortcut;

        // Map textual modifiers to symbols for a professional look
        std::string display_text = shortcut;
        auto replace = [&](const std::string &from, const std::string &to)
        {
            size_t pos = 0;
            while ((pos = display_text.find(from, pos)) != std::string::npos)
            {
                display_text.replace(pos, from.length(), to);
                pos += to.length();
            }
        };

        replace("Ctrl", "⌃");
        replace("Shift", "⇧");
        replace("+", ""); // Usually shortcuts are displayed as ⌃⇧S rather than ⌃+⇧+S

        m_shortcut_label->set_text(display_text);
    }

    void MenuItem::set_icon(const std::string &icon_path)
    {
        auto icon = std::make_unique<Icon>();
        m_icon = icon.get();
        m_icon->set_icon_name(icon_path);
        m_icon->set_icon_size(16);
        m_icon->set_position_type(FREE);
        add_child(std::move(icon));
    }

    void MenuItem::set_has_submenu(bool has_submenu)
    {
        m_has_submenu = has_submenu;
    }

    void MenuItem::set_enabled(bool enabled)
    {
        Widget::set_enabled(enabled);
        invalidate();
    }

    void MenuItem::set_selected(bool selected)
    {
        if (m_selected != selected)
        {
            if (selected && !is_enabled())
                return;

            m_selected = selected;
            invalidate();
        }
    }

    void MenuItem::set_content_widget(std::unique_ptr<Widget> widget)
    {
        // This is a bit tricky since we already have a default content
        // We might want to remove the old one if we were doing this for real
        m_content = widget.get();
        m_content->set_position_type(FREE);
        add_child(std::move(widget));
    }

    void MenuItem::set_submenu(std::unique_ptr<Menu> submenu)
    {
        m_submenu = std::move(submenu);
        set_has_submenu(m_submenu != nullptr);
    }

    int MenuItem::preferred_width() const
    {
        if (!application() || !application()->theme_manager)
            return 200; // Fallback

        auto font = application()->theme_manager->get_font("window");

        // Create a temporary context just for measuring text (4 bytes for 1x1 ARGB pixel)
        uint32_t tmp_pixel = 0;
        CairoGraphicContext measure_ctx(nullptr, &tmp_pixel, 1, 1);

        int icon_width = (m_icon || m_reserve_icon_space) ? 24 : 0;
        int arrow_width = m_has_submenu ? 20 : 0;
        int padding = 10;                           // Left + right padding
        int gap = m_shortcut_text.empty() ? 0 : 20; // Increased gap
        int slack = 15;                             // Extra room for safety

        // Measure label text width
        int label_w = 0;
        if (auto *label = dynamic_cast<Label *>(m_content))
        {
            auto metrics =
                measure_ctx.getTextMetrics(label->text().c_str(), font.family.c_str(), font.size,
                                           FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            label_w = static_cast<int>(std::ceil(metrics.width));
        }

        // Measure shortcut text width
        int shortcut_w = 0;
        if (!m_shortcut_text.empty())
        {
            auto metrics =
                measure_ctx.getTextMetrics(m_shortcut_text.c_str(), font.family.c_str(), font.size,
                                           FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            shortcut_w = static_cast<int>(std::ceil(metrics.width));
        }

        return padding + icon_width + label_w + gap + shortcut_w + arrow_width + padding + slack;
    }

    void MenuItem::calculate_layout()
    {
        // Essential: Refresh m_start_draw_x/y from parent
        Widget::calculate_layout();

        int icon_width = (m_icon || m_reserve_icon_space) ? 24 : 0;
        int arrow_width = m_has_submenu ? 20 : 0; // Fix: only subtract if we have an arrow
        int padding = 12;                         // Increased padding
        int shortcut_gap = m_shortcut_text.empty() ? 0 : 20; // Increased gap

        // Calculate dynamic shortcut width
        int shortcut_width = 0;
        if (!m_shortcut_text.empty() && application() && application()->theme_manager)
        {
            auto font = application()->theme_manager->get_font("window");
            uint32_t tmp_pixel = 0;
            CairoGraphicContext measure_ctx(nullptr, &tmp_pixel, 1, 1);
            auto metrics =
                measure_ctx.getTextMetrics(m_shortcut_text.c_str(), font.family.c_str(), font.size,
                                           FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            shortcut_width = static_cast<int>(std::ceil(metrics.width));
        }

        // Reserve space for icon if this item has one OR if any sibling does
        int content_x = padding;
        if (m_icon)
        {
            m_icon->set_position(m_start_draw_x + padding, m_start_draw_y + (m_height - 16) / 2);
            m_icon->set_size(16, 16);
            content_x += 24; // Standard icon slot
            m_icon->calculate_layout();
        }
        else if (m_reserve_icon_space)
        {
            content_x += 24;
        }

        int available_content_width = m_width - content_x - shortcut_gap - shortcut_width -
                                      arrow_width - padding + 5; // +5 safety buffer

        m_content->set_position(m_start_draw_x + content_x, m_start_draw_y);
        m_content->set_size(available_content_width, m_height);
        m_content->calculate_layout();

        if (!m_shortcut_text.empty() && m_shortcut_label)
        {
            m_shortcut_label->set_position(
                m_start_draw_x + m_width - arrow_width - shortcut_width - padding, m_start_draw_y);
            m_shortcut_label->set_size(shortcut_width, m_height);
            m_shortcut_label->calculate_layout();
        }
    }

    void MenuItem::draw(GraphicsContext &gc)
    {

        if (m_selected)
        {
            // macOS selection gradient (Blue)
            Color c1(0.2f, 0.45f, 0.9f, 1.0f);
            Color c2(0.1f, 0.35f, 0.85f, 1.0f);
            gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_width, m_height, c1, c2,
                                      true);

            // White text for selected items
            if (auto *label = dynamic_cast<Label *>(m_content))
            {
                label->set_text_color({1.0f, 1.0f, 1.0f, 1.0f});
            }
            if (m_shortcut_label)
            {
                m_shortcut_label->set_text_color({1.0f, 1.0f, 1.0f, 0.8f});
            }
        }
        else
        {
            float alpha = is_enabled() ? 1.0f : 0.4f;
            // Default text color
            if (auto *label = dynamic_cast<Label *>(m_content))
            {
                label->set_text_color({0.0f, 0.0f, 0.0f, alpha});
            }
            if (m_shortcut_label)
            {
                m_shortcut_label->set_text_color({0.4f, 0.4f, 0.4f, alpha});
            }
            if (m_icon)
            {
                // We don't have a specific set_opacity for Icon, but we could if needed.
                // For now, text already communicates the state.
            }
        }

        // Draw children (items) via base class
        Widget::draw(gc);

        if (m_has_submenu)
        {
            // Draw submenu arrow
            int arrow_size = 6;
            int ax = m_start_draw_x + m_width - 15;
            int ay = m_start_draw_y + (m_height - arrow_size * 2) / 2;

            gc.setColor(m_selected ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.2f, 0.2f, 0.2f, 1.0f));
            std::vector<PolygonPoint> points;
            points.push_back({ax, ay, 0});
            points.push_back({ax + arrow_size, ay + arrow_size, 0});
            points.push_back({ax, ay + arrow_size * 2, 0});
            gc.fillPolygon(points);
        }
    }

} // namespace horizon
