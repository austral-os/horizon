#include <horizon/Combo.hpp>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    Combo::Combo() : AirObject()
    {
        set_size(150, 30);
        
        when_click.connect([this](MouseButtonEventContext &) {
            on_click();
        });
    }

    Combo::~Combo() = default;

    void Combo::add_item(const std::string &id, const std::string &text, const std::string &icon_name)
    {
        m_items.push_back({id, text, icon_name});
        if (m_selected_index == -1)
        {
            m_selected_index = 0;
            invalidate();
        }
        update_menu();
    }

    void Combo::clear_items()
    {
        m_items.clear();
        m_selected_index = -1;
        m_menu.reset();
        invalidate();
    }

    void Combo::set_selected_item_by_id(const std::string &id)
    {
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            if (m_items[i].id == id)
            {
                if (m_selected_index != (int)i)
                {
                    m_selected_index = (int)i;
                    invalidate();
                }
                return;
            }
        }
    }

    const ComboItem* Combo::selected_item() const
    {
        if (m_selected_index >= 0 && m_selected_index < (int)m_items.size())
        {
            return &m_items[m_selected_index];
        }
        return nullptr;
    }

    void Combo::update_menu()
    {
        m_menu = std::make_unique<Menu>();
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            auto *item = m_menu->add_item(m_items[i].text, "", m_items[i].id);
            if (!m_items[i].icon_name.empty())
            {
                item->set_icon(m_items[i].icon_name);
            }
            
            item->when_click.connect([this, i](MouseButtonEventContext &) {
                handle_selection((int)i);
            });
        }
    }

    void Combo::handle_selection(int index)
    {
        if (index >= 0 && index < (int)m_items.size())
        {
            m_selected_index = index;
            invalidate();

            ComboItemSelectedContext ctx;
            ctx.sender = this;
            ctx.item = m_items[index];
            when_item_selected.run(ctx);
        }
    }

    void Combo::on_click()
    {
        if (!m_menu || m_items.empty()) return;

        if (auto *win = dynamic_cast<WaylandWindow *>(application()))
        {
            // Match menu width to combo width
            m_menu->set_min_width(m_width);
            m_menu->set_max_width(m_width);
            m_menu->calculate_layout();

            // Position the menu below the combo
            win->show_context_menu(m_menu.get(), m_x, m_y + m_height);
        }
    }

    void Combo::calculate_layout()
    {
        AirObject::calculate_layout();
    }

    void Combo::draw(GraphicsContext &gc)
    {
        // 1. Draw the base AirObject appearance
        AirObject::draw(gc);

        auto *tm = application()->theme_manager.get();
        auto theme_font = tm->get_font("window");
        
        // 2. Draw selected item text/icon
        const ComboItem *selected = selected_item();
        int margin = 8;
        int arrow_area_width = 30;
        int text_x = m_start_draw_x + margin;
        
        if (selected)
        {
            gc.setDrawFont(theme_font.family.c_str(), theme_font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            gc.setColor(tm->get_color("window_text"));
            
            std::string display_text = selected->text;
            int max_text_width = m_width - margin - arrow_area_width;

            TextMetrics tm_text = gc.getTextMetrics(display_text.c_str(), theme_font.family.c_str(), theme_font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            
            if (tm_text.width > max_text_width)
            {
                display_text += "...";
                while (display_text.length() > 3)
                {
                    auto m = gc.getTextMetrics(display_text.c_str(), theme_font.family.c_str(), theme_font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                    if (m.width <= max_text_width)
                        break;
                    display_text.erase(display_text.length() - 4, 1);
                }
            }

            int text_y = m_start_draw_y + (m_height + tm_text.height) / 2 - 2;
            gc.drawText(text_x, text_y, display_text.c_str());
        }

        // 3. Draw arrows on the right
        int arrow_x = m_start_draw_x + m_width - arrow_area_width;
        int centerY = m_start_draw_y + m_height / 2;

        gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.1f));
        gc.drawLine(arrow_x, m_start_draw_y + 5, arrow_x, m_start_draw_y + m_height - 5, 1.0f);

        gc.setColor(Color(0.4f, 0.4f, 0.4f, 1.0f));
        
        // Down arrow
        gc.drawLine(arrow_x + 10, centerY + 1, arrow_x + 15, centerY + 5, 1.5f);
        gc.drawLine(arrow_x + 15, centerY + 5, arrow_x + 20, centerY + 1, 1.5f);
        
        // Up arrow
        gc.drawLine(arrow_x + 10, centerY - 1, arrow_x + 15, centerY - 5, 1.2f);
        gc.drawLine(arrow_x + 15, centerY - 5, arrow_x + 20, centerY - 1, 1.2f);
    }
} // namespace horizon
