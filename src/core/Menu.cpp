#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Menu.hpp>

namespace horizon
{

    Menu::Menu() : Widget()
    {
        // Default appearance - Solid white-ish as requested
        set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
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

    MenuItem *Menu::add_item(const std::string &text, const std::string &shortcut)
    {
        auto item = std::make_unique<MenuItem>(text);
        if (!shortcut.empty())
            item->set_shortcut(shortcut);
        auto *ptr = item.get();
        add_item(std::move(item));
        return ptr;
    }

    void Menu::calculate_layout()
    {
        // Update m_start_draw_x/y based on current m_x/y
        Widget::calculate_layout();

        int padding_top = 10;
        int padding_bottom = 10;
        int current_y = 1 + padding_top; // 1px border + padding
        int max_w = m_min_width;

        // Detect if any MenuItem has an icon (for consistent left margin)
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

        // Propagate icon space reservation to all items
        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<MenuItem *>(child.get()))
            {
                item->set_reserve_icon_space(any_has_icon);
            }
        }

        // First pass: Determine max width needed based on actual content
        for (const auto &child : m_children)
        {
            if (auto *item = dynamic_cast<MenuItem *>(child.get()))
            {
                max_w = std::max(max_w, item->preferred_width() + 2);
            }
            else
            {
                max_w = std::max(max_w, child->width() + 2);
            }
        }

        // Second pass: Layout children using absolute coordinates
        for (auto &child : m_children)
        {
            child->set_position(m_start_draw_x + 1, m_start_draw_y + current_y);
            child->set_size(max_w - 2, child->height());
            child->calculate_layout(); // Explicit recursive call for stable positioning
            current_y += child->height();
        }

        // Update size through set_size to trigger invalidation if changed
        set_size(max_w, current_y + padding_bottom);
        // Base coordinate refresh is handled within set_size -> invalidate or explicitly
        Widget::calculate_layout();
    }

    void Menu::close_submenus()
    {
        if (m_active_submenu)
        {
            m_active_submenu->close_submenus();
            m_active_submenu->set_visible(false);
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
            m_active_submenu->set_visible(true);
            m_active_submenu->invalidate();

            // Positioning is tricky here because MenuItem doesn't know its own screen position
            // reliably without Widget::calculate_layout having been called recently. But we can try
            // to find the item that triggered this. Actually, for now we let the MenuItem pass its
            // position if needed, or we just find which child is hovered.

            for (const auto &child : m_children)
            {
                if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                {
                    if (item->is_selected())
                    {
                        // Position submenu to the right of the item
                        m_active_submenu->set_position(item->x() + item->width() - 2, item->y());
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
                // Request a full repaint of the entire surface.
                // This is necessary because Cairo's pushGroup/popGroup prevents
                // CLEAR operations from reaching the actual surface buffer.
                // A full repaint re-renders everything from scratch, naturally
                // omitting the hidden menu.
                if (application())
                {
                    application()->invalidate(nullptr);
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

        // 3. Draw children with clipping
        CornerRadius radius(0, 0, 10, 10);
        ctx.save();
        ctx.clipRoundedRect(m_start_draw_x, m_start_draw_y, m_width, m_height, radius);

        for (const auto &child : m_children)
        {
            if (child->is_visible())
            {
                child->render(ctx, cx, cy, cw, ch, should_draw);
            }
        }

        ctx.restore();

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
