#include "horizon/TabCollection.hpp"
#include "horizon/Application.hpp"
#include "horizon/Button.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Logger.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/Widget.hpp"
#include <algorithm>
#include <memory>

namespace horizon
{

    TabCollection::TabButton::TabButton(TabCollection *owner, int index, const std::string &title)
        : Widget(), m_owner(owner), m_index(index), m_title(title)
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_focusable(true);

        when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                m_owner->set_current_tab(m_index);
                m_owner->when_tab_selected.run(m_index);
            });

        if (m_owner->closable_tabs())
        {
            auto close_icon = std::make_unique<Icon>();
            close_icon->set_icon_name("window-close");
            close_icon->set_icon_size(16);
            close_icon->set_fixed_size(32);
            close_icon->set_margin(5);

            m_close_button = close_icon.get();

            m_close_button->when_click.connect([this](MouseButtonEventContext &ctx)
                                               { m_owner->when_tab_close_requested.run(m_index); });

            add_child(std::move(close_icon));
        }
    }

    void TabCollection::TabButton::set_active(bool active)
    {
        m_active = active;
        invalidate();
    }

    void TabCollection::TabButton::set_title(const std::string &title)
    {
        m_title = title;
        invalidate();
    }

    void TabCollection::TabButton::draw(GraphicsContext &ctx)
    {

        auto *tm = application()->theme_manager.get();

        Color c1 = tm->get_color("tab_button_1");
        Color c2 = tm->get_color("tab_button_2");

        // Draw background if active
        if (m_active)
        {
            ctx.fillLinearGradientRect(x(), y(), width(), height(), c1, c2, true);
        }
        else if (is_hovered())
        {
            ctx.setColor(Color(1.0f, 1.0f, 1.0f, 0.1f));
            ctx.fillRect(x(), y(), width(), height());
        }

        // Draw separator line on the right
        ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.2f));
        ctx.drawLine(x() + width() - 1, y() + 8, x() + width() - 1, y() + height() - 8, 1.0f);

        // Draw text centered
        auto font = tm->get_font("window");
        ctx.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL,
                        m_active ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

        ctx.setColor(Color(0.2f, 0.2f, 0.2f, 1.0f));

        std::string display_title = m_title;
        int max_text_width = width() - 20; // 10px padding on each side
        TextMetrics metrics =
            ctx.getTextMetrics(display_title.c_str(), font.family.c_str(), font.size,
                               FONT_SLANT_NORMAL, m_active ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

        if (metrics.width > max_text_width)
        {
            display_title += "...";
            while (display_title.length() > 3)
            {
                metrics = ctx.getTextMetrics(display_title.c_str(), font.family.c_str(), font.size,
                                             FONT_SLANT_NORMAL,
                                             m_active ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);
                if (metrics.width <= max_text_width)
                    break;
                display_title.erase(display_title.length() - 4, 1);
            }
        }

        int text_offset = 0;
        if (m_close_button && m_close_button->is_effectively_visible())
        {
            text_offset = 14; // Half of container width (28px)
        }

        int tx = x() + (width() - metrics.width) / 2 + text_offset;
        int ty = y() + (height() + metrics.height) / 2 - 2;

        ctx.drawText(tx, ty, display_title.c_str());
    }

    int TabCollection::TabButton::preferred_width() const
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");
        // Get text width from application's graphics context or a temporary one
        // For now, let's assume a reasonable width or provide a way to calculate it
        return 150; // Default width for tabs
    }

    TabCollection::TabCollection() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(FILL);

        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(34);
        m_header = header.get();
        add_child(std::move(header));

        auto container = std::make_unique<Widget>();
        container->set_position_type(FILL);
        m_container = container.get();
        add_child(std::move(container));

        // Create the add button
        auto add_btn = std::make_unique<Button<SolidObject>>();
        add_btn->set_text("+");
        add_btn->set_fixed_size(34);
        add_btn->set_background_color(Color(1.0f, 1.0f, 1.0f, 0.1f));
        add_btn->when_mouse_press.connect([this](MouseButtonEventContext &ctx)
                                          { when_add_tab_clicked.run(ctx); });
        m_add_button = add_btn.get();
        m_header->add_child(std::move(add_btn));
    }

    int TabCollection::add_tab(const std::string &title, std::unique_ptr<Widget> body)
    {
        int index = (int)m_tabs.size();

        // Hide body initially
        body->set_visible(false);
        body->set_position_type(FILL);

        // Ownership is transferred to m_container, but we keep a raw pointer for access.
        Widget *body_ptr = body.get();
        m_container->add_child(std::move(body));

        // Store tab page with raw pointer.
        m_tabs.push_back({title, body_ptr});

        // Create and add tab button to header
        auto tab_btn = std::make_unique<TabButton>(this, index, title);
        m_header->add_child_at(index, std::move(tab_btn));

        if (m_current_tab == -1)
        {
            set_current_tab(0);
        }

        int tab_index = index;
        when_tab_added.run(tab_index);
        int count = (int)m_tabs.size();
        when_items_changed.run(count);

        if (m_smart_header)
        {
            show_header(count > 1);
        }

        invalidate();
        return index;
    }

    void TabCollection::remove_tab(int index)
    {
        if (index < 0 || index >= (int)m_tabs.size())
            return;

        // Remove from header and container
        m_header->remove_child_at(index);
        m_container->remove_child_at(index);

        m_tabs.erase(m_tabs.begin() + index);

        // Correct indices in remaining tab buttons
        for (int i = 0; i < (int)m_header->children().size() - 1; ++i)
        {
            if (auto *btn = dynamic_cast<TabButton *>(m_header->children()[i].get()))
            {
                btn->set_index(i);
            }
        }

        if (m_current_tab >= (int)m_tabs.size())
        {
            set_current_tab((int)m_tabs.size() - 1);
        }
        else
        {
            set_current_tab(m_current_tab); // Refresh visibility
        }

        int count = (int)m_tabs.size();
        when_items_changed.run(count);

        if (m_smart_header)
        {
            show_header(count > 1);
        }

        invalidate();
    }

    void TabCollection::set_tab_title(int index, const std::string &title)
    {
        if (index < 0 || index >= (int)m_tabs.size())
            return;
        m_tabs[index].title = title;
        if (auto *btn = dynamic_cast<TabButton *>(m_header->children()[index].get()))
        {
            btn->set_title(title);
        }
    }

    void TabCollection::show_header(bool visible)
    {
        if (m_header)
        {
            m_header->set_visible(visible);
            invalidate();
        }
    }

    void TabCollection::set_smart_header(bool enabled)
    {
        m_smart_header = enabled;
        if (m_smart_header)
        {
            show_header(m_tabs.size() > 1);
        }
        else
        {
            show_header(true);
        }
    }

    void TabCollection::set_closable_tabs(bool enabled)
    {
        m_closable_tabs = enabled;
    }

    void TabCollection::set_current_tab(int index)
    {
        if (index < 0 || index >= (int)m_tabs.size())
            return;

        m_current_tab = index;

        // Update button states
        for (int i = 0; i < (int)m_header->children().size() - 1; ++i)
        {
            if (auto *btn = dynamic_cast<TabButton *>(m_header->children()[i].get()))
            {
                btn->set_active(i == m_current_tab);
            }
        }

        // Update body visibility
        for (int i = 0; i < (int)m_container->children().size(); ++i)
        {
            m_container->children()[i]->set_visible(i == m_current_tab);
            if (i == m_current_tab)
            {
                m_container->children()[i]->invalidate();
            }
        }

        invalidate();
    }

    Widget *TabCollection::current_tab_body() const
    {
        if (m_current_tab < 0 || m_current_tab >= (int)m_container->children().size())
            return nullptr;
        return m_container->children()[m_current_tab].get();
    }

    void TabCollection::render(GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force)
    {
        // Ensure the add button is always at the end and visible if there's space
        update_layout();
        Widget::render(ctx, cx, cy, cw, ch, force);
    }

    void TabCollection::draw(GraphicsContext &ctx)
    {

        auto *tm = application()->theme_manager.get();

        Color c1 = tm->get_color("tab_header_1");
        Color c2 = tm->get_color("tab_header_2");

        ctx.fillLinearGradientRect(m_header->x(), m_header->y(), m_header->width(),
                                   m_header->height(), c1, c2, true);

        // Draw a bottom border for the header
        ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.2f));
        ctx.drawLine(m_header->x(), m_header->y() + m_header->height() - 1,
                     m_header->x() + m_header->width(), m_header->y() + m_header->height() - 1,
                     1.0f);

        Widget::draw(ctx);
    }

    void TabCollection::update_layout()
    {
        // Layout tab buttons and the add button
        if (m_tabs.empty())
            return;

        int available_width = m_header->width() - m_add_button->width();
        int tab_width = std::min(200, available_width / (int)m_tabs.size());

        for (int i = 0; i < (int)m_header->children().size() - 1; ++i)
        {
            m_header->children()[i]->set_width(tab_width);
        }
    }

} // namespace horizon
