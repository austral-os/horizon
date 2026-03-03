#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    Toolbar::Toolbar(std::string title) : Titlebar(std::move(title))
    {
        // 1. Capture existing children (standard titlebar buttons and spacer)
        // Titlebar constructor already ran and added [Spacer, Close, Min, Max] to this->m_children.
        std::vector<std::unique_ptr<Widget>> titlebar_elements;
        for (auto &child : m_children)
        {
            titlebar_elements.push_back(std::move(child));
        }
        m_children.clear();

        // 2. Configure Toolbar itself as Vertical
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_fixed_size(68); // 34 (top) + 34 (bottom)

        // 3. Create Top Row (for standard buttons/spacer)
        auto top_row = std::make_unique<Widget>();
        top_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        top_row->set_position_type(FILL);
        top_row->set_fixed_size(34);
        top_row->set_margin(9); // Centra los botones de 16px en la fila de 34px
        top_row->set_spacing(8);
        m_top_row = top_row.get();

        // Move standard elements to top_row
        for (auto &child : titlebar_elements)
        {
            m_top_row->add_child(std::move(child));
        }

        // 4. Create Bottom Row (for custom widgets)
        auto bottom_row = std::make_unique<Widget>();
        bottom_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_row->set_position_type(FILL);
        bottom_row->set_fixed_size(34);
        bottom_row->set_margin(4); // Pequeño margen para los widgets de la toolbar
        bottom_row->set_spacing(6);
        m_bottom_row = bottom_row.get();

        // 5. Add rows to Toolbar
        add_child(std::move(top_row));
        add_child(std::move(bottom_row));
    }

    void Toolbar::add_toolbar_widget(std::unique_ptr<Widget> widget)
    {
        if (m_bottom_row)
        {
            m_bottom_row->add_child(std::move(widget));
        }
    }

    void Toolbar::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        Color title_brd = tm->get_color("titlebar_border");
        Color title_bg1 = tm->get_color("titlebar_bg1");
        Color title_bg2 = tm->get_color("titlebar_bg2");

        // Background for the whole toolbar
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1,
                                  m_available_draw_width - 2, m_available_draw_height, title_bg2,
                                  title_bg1, true, CornerRadius(10, 10, 0, 0));

        // Bottom border
        gc.setColor(title_brd);
        gc.drawRect(0, m_height - 1, m_width, 0, 0, 0.8f);

        // Draw the title text in the TOP half only
        auto font = tm->get_font("titlebar");
        Color title_fg = tm->get_color("titlebar_fg");
        TextMetrics metrics = gc.getTextMetrics(m_title.c_str(), font.family.c_str(), font.size,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);

        int text_x = (m_width / 2) - (metrics.width / 2);
        // Center text in top half (first 34px)
        int text_y = (34 / 2) + (metrics.height / 2) - 1;

        gc.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
        gc.setColor(title_fg);
        gc.drawText(text_x, text_y, m_title.c_str());
    }

} // namespace horizon
