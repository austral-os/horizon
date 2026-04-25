#pragma once

#include <horizon/Application.hpp>
#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @class TableRow
     * @brief A simple horizontal container for table cells with background support.
     */
    class TableRow : public Widget
    {
    public:
        TableRow() : Widget()
        {
            m_layout_type = WIDGET_LAYOUT_HORIZONTAL;
            m_position_type = FILL;
            m_spacing = 0;
            m_margin = 0;
        }

        void set_alternate(bool alt)
        {
            m_is_alternate = alt;
            invalidate();
        }

        void set_selected(bool sel)
        {
            m_is_selected = sel;
            invalidate();
        }

    protected:
        void draw(GraphicsContext &gc) override
        {
            auto *tm = application()->theme_manager.get();
            Color bg;
            Color fg;

            if (m_is_selected)
            {
                // Blue gradient selection (macOS Aqua style)
                Color c1(0.32f, 0.61f, 0.90f, 1.0f); // Top
                Color c2(0.11f, 0.45f, 0.81f, 1.0f); // Bottom
                gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, c1, c2, true);
                
                fg = tm->get_color("table_row_selected_fg");
                apply_text_color_recursive(this, fg);
                Widget::draw(gc);
                return;
            }
            else if (m_is_alternate)
            {
                bg = tm->get_color("table_row_alternate");
                fg = tm->get_color("table_row_fg");
            }
            else
            {
                bg = tm->get_color("table_row");
                fg = tm->get_color("table_row_fg");
            }

            gc.setColor(bg);
            gc.fillRect(m_x, m_y, m_width, m_height);

            // Propagate text color to children (mostly Labels)
            apply_text_color_recursive(this, fg);

            Widget::draw(gc);
        }

    private:
        void apply_text_color_recursive(Widget *w, Color fg)
        {
            if (!w)
                return;

            // Using dynamic_cast to find Labels and set their color
            if (auto *lbl = dynamic_cast<Label *>(w))
            {
                lbl->set_text_color(fg);
            }

            for (auto &child : w->children())
            {
                apply_text_color_recursive(child.get(), fg);
            }
        }

        bool m_is_alternate{false};
        bool m_is_selected{false};
    };
} // namespace horizon
