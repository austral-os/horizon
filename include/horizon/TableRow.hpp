#pragma once

#include <horizon/Application.hpp>
#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
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

            if (m_is_selected)
                bg = tm->get_color("table_row_selected");
            else if (m_is_alternate)
                bg = tm->get_color("table_row_alternate");
            else
                bg = tm->get_color("table_row");

            gc.setColor(bg);
            gc.fillRect(m_x, m_y, m_width, m_height);

            Widget::draw(gc);
        }

    private:
        bool m_is_alternate{false};
        bool m_is_selected{false};
    };
} // namespace horizon
