#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/SystemInfo.hpp>
#include <cmath>

namespace horizon
{

    Menu::Menu() : Widget()
    {
        // Default appearance - Solid white-ish as requested
        set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));

        // Stop propagation of mouse events to prevent background clicks
        when_mouse_press.connect([](MouseButtonEventContext &ev) { ev.stop_propagation = true; });

        m_max_menu_height = 600; // Default, will be overridden by WaylandWindow on show

        when_mouse_wheel.connect([this](MouseWheelEventContext &ctx) {
            double delta = ctx.dy * 20.0; // Smoother scroll
            double old_scroll = m_scroll_y;
            m_scroll_y += delta;
            
            double max_scroll = std::max(0.0, m_total_content_height - (double)m_height + 10.0);
            if (m_scroll_y < 0.0) m_scroll_y = 0.0;
            if (m_scroll_y > max_scroll) m_scroll_y = max_scroll;
            
            if (std::abs(m_scroll_y - old_scroll) > 0.001) {
                invalidate();
            }
        });
    }

    void Menu::add_item(std::unique_ptr<MenuItem> item)
    {
        item->set_position_type(FREE);
        add_child(std::move(item));
        calculate_layout();
    }

    void Menu::add_separator()
    {
        auto separator = std::make_unique<MenuSeparator>();
        separator->set_position_type(FREE);
        add_child(std::move(separator));
        calculate_layout();
    }

    MenuItem *Menu::add_item(const std::string &text, const std::string &shortcut,
                             const std::string &item_id)
    {
        auto item = std::make_unique<MenuItem>(text);
        if (!shortcut.empty())
            item->set_shortcut(shortcut);
        if (!item_id.empty())
            item->set_id(item_id);
        auto *ptr = item.get();
        add_item(std::move(item));
        return ptr;
    }

    void Menu::calculate_layout()
    {
        Widget::calculate_layout();

        int max_w = m_min_width; // Start with m_min_width to enforce minimums (like in Combos)
        int padding_top = 4;
        int padding_bottom = 4;
        int current_y = padding_top;

        // First pass: Determine icons presence
        bool any_has_icon = false;
        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<MenuItem *>(child.get()))
            {
                if (item->has_icon())
                {
                    any_has_icon = true;
                    break;
                }
            }
        }

        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                item->set_reserve_icon_space(any_has_icon);
        }

        // Second pass: Determine max width needed
        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                max_w = std::max(max_w, item->preferred_width() + 2);
        }

        if (max_w < 100) max_w = 200;

        // Enforce max width if set
        if (m_max_width > 0)
            max_w = std::min(max_w, m_max_width);

        // Third pass: Layout children with ABSOLUTE coordinates
        for (auto &child : m_children)
        {
            int h = child->preferred_height();
            if (h <= 0) h = 24;

            child->set_position(m_start_draw_x + 1, m_start_draw_y + current_y);
            child->set_size(max_w - 2, h);
            child->calculate_layout();
            current_y += h;
        }

        m_total_content_height = current_y + padding_bottom;
        int final_h = (int)m_total_content_height;
        if (m_max_menu_height > 0 && final_h > m_max_menu_height)
            final_h = m_max_menu_height;

        set_size(max_w, final_h);
    }

    Widget *Menu::hit_test(int x, int y)
    {
        // First check if we have an active submenu and if the hit is there
        if (m_active_submenu && m_active_submenu->is_visible())
        {
            if (Widget *hit = m_active_submenu->hit_test(x, y))
                return hit;
        }

        if (x < m_x || y < m_y || x > m_x + m_width || y > m_y + m_height)
            return nullptr;

        int local_y = y + (int)m_scroll_y;

        for (auto &child : m_children)
        {
            // We want the direct child (the MenuItem) to be returned, 
            // not its internal sub-widgets (like Labels), because the 
            // selection handlers are connected to the MenuItem itself.
            if (child->hit_test(x, local_y))
                return child.get();
        }
        return this;
    }

    int Menu::calculate_cascade_width() const
    {
        int max_sub_cascade = 0;
        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<const MenuItem *>(child.get()))
            {
                if (item->submenu())
                {
                    Menu *sub = item->submenu();

                    // Force a layout pass at (0,0) if the submenu hasn't been laid out
                    // yet. This gives us accurate width without needing to open the submenu.
                    if (sub->width() == 0)
                    {
                        sub->set_position(0, 0);
                        const_cast<Menu *>(sub)->calculate_layout();
                    }

                    int sub_w = sub->width();
                    if (sub_w < sub->m_min_width) sub_w = sub->m_min_width;
                    int nested = sub->calculate_cascade_width();
                    max_sub_cascade = std::max(max_sub_cascade, sub_w + nested);
                }
            }
        }
        return max_sub_cascade;
    }

    void Menu::close_submenus()
    {
        if (m_active_submenu)
        {
            m_active_submenu->close_submenus();
            m_active_submenu->set_visible(false);
            // Crucial: Unset parent before clearing pointer
            if (m_active_submenu->m_parent == this) {
                m_active_submenu->m_parent = nullptr;
            }
            m_active_submenu = nullptr;
            invalidate(); // Ensure the area where the submenu was is repainted
        }
    }

    void Menu::set_active_submenu(Menu *menu)
    {
        if (m_active_submenu == menu)
            return;

        close_submenus();

        if (menu)
        {
            m_active_submenu = menu;
            m_active_submenu->m_parent = this;
            m_active_submenu->set_application_recursive(application());
            m_active_submenu->set_visible(true);
            m_active_submenu->invalidate();

            for (const auto &child : m_children)
            {
                if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                {
                    if (item->is_selected())
                    {
                        // Do a preliminary layout to know the submenu's dimensions
                        m_active_submenu->set_position(item->x() + item->width() - 2, item->y());
                        m_active_submenu->calculate_layout();

                        // --- Edge detection & smart repositioning ---
                        int sub_w = m_active_submenu->width();
                        int sub_h = m_active_submenu->height();

                        int pos_x = item->x() + item->width() - 2;
                        int pos_y = item->y();

                        // Get available surface space
                        int surface_w = 0, surface_h = 0;
                        if (application() && application()->w_surface())
                        {
                            surface_w = application()->w_surface()->width();
                            surface_h = application()->w_surface()->height();
                        }

                        // Flip horizontally: if submenu right edge exceeds surface width, open to the left
                        if (surface_w > 0 && pos_x + sub_w > surface_w)
                        {
                            pos_x = item->x() - sub_w + 2;
                            if (pos_x < 0) pos_x = 0;
                        }

                        // Flip vertically: if submenu bottom exceeds surface height, push it up
                        if (surface_h > 0 && pos_y + sub_h > surface_h)
                        {
                            pos_y = surface_h - sub_h;
                            if (pos_y < 0) pos_y = 0;
                        }

                        m_active_submenu->set_position(pos_x, pos_y);
                        m_active_submenu->calculate_layout();
                        break;
                    }
                }
            }
        }
    }

    void Menu::render(GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force)
    {
        // Handle visibility transition: if was visible but now hidden, clear the area
        if (!m_visible)
        {
            if (m_was_visible)
            {
                if (application())
                {
                    // Only force full repaint for transparent surfaces (LayerApplication).
                    // Regular applications have opaque backgrounds that naturally cover old pixels.
                    if (application()->is_transparent_surface())
                    {
                        application()->invalidate(nullptr); // Full surface repaint
                    }
                    else
                    {
                        invalidate(); // Normal selective repaint
                    }
                }
                m_was_visible = false;
            }
            return;
        }

        m_was_visible = true;

        // 1. Ensure layout is calculated before anything else
        calculate_layout();

        bool intersects =
            !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);
        if (!intersects)
            return;

        bool should_draw = m_dirty || force || m_child_dirty;

        // 2. Draw the menu container (background and border)
        if (should_draw)
        {
            draw(ctx);
        }

        // 3. Draw children with clipping and scroll translation
        CornerRadius radius(0, 0, 10, 10);
        ctx.save();
        ctx.clipRoundedRect(m_start_draw_x, m_start_draw_y, m_width, m_height, radius);
        
        ctx.save();
        ctx.translate(0, -m_scroll_y);

        for (const auto &child : m_children)
        {
            if (child->is_visible())
            {
                // Translate the dirty region check for children to account for scroll
                child->render(ctx, cx, cy + (int)m_scroll_y, cw, ch, should_draw);
            }
        }

        ctx.restore(); // Restore translation
        ctx.restore(); // Restore clipping

        // 4. Draw active submenu (outside the parent's clipping and translation)
        if (m_active_submenu && m_active_submenu->is_visible())
        {
            // Ensure the submenu is rendered into the same context but with its own absolute coordinates.
            // We pass the same dirty region as the parent.
            m_active_submenu->render(ctx, cx, cy, cw, ch, force || should_draw);
        }

        m_dirty = false;
        m_child_dirty = false;
    }

    void Menu::draw(GraphicsContext &gc)
    {
        // macOS Menu style
        // Straight top corners, rounded bottom corners
        CornerRadius radius(0, 0, 10, 10);

        // Shadow/Border
        gc.setColor(Color(0.7f, 0.7f, 0.7f, 0.8f));
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height, radius, 1.0f);

        // Fill background
        gc.setColor(background_color());
        gc.fillRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, m_height - 2, radius);
    }

} // namespace horizon
