#include "horizon/Spacer.hpp"
#include <algorithm>
#include <cmath>
#include <horizon/EventsManager.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/RibbonToolbar.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{

    // --- RibbonTabButton ---
    class RibbonTabButton : public Widget
    {
    public:
        RibbonTabButton(const std::string &title, int index, RibbonToolbar *toolbar)
            : m_title(title), m_index(index), m_toolbar(toolbar)
        {

            when_mouse_press.connect(
                [this](MouseButtonEventContext &ev)
                {
                    if (ev.button == 0x110)
                    {
                        m_toolbar->set_active_tab(m_index);
                    }
                });
        }

        void set_active(bool active)
        {
            if (m_active != active)
            {
                m_active = active;
                invalidate();
            }
        }

        int preferred_width() const override
        {
            return m_cached_width > 0 ? m_cached_width : 80;
        }

        void draw(GraphicsContext &ctx) override
        {
            Color fg = theme_manager()->get_color(m_active ? "window_fg" : "sidebar_fg");

            ctx.setColor(fg);
            auto font = theme_manager()->get_font("ui");
            ctx.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL,
                            m_active ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

            TextMetrics metrics = ctx.getTextMetrics(
                m_title.c_str(), font.family.c_str(), font.size, FONT_SLANT_NORMAL,
                m_active ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

            // Usar siempre métricas de peso normal para Y → mismo nivel en todos los tabs
            TextMetrics ref_metrics =
                ctx.getTextMetrics(m_title.c_str(), font.family.c_str(), font.size,
                                   FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

            int text_x = m_start_draw_x + (m_width - metrics.width) / 2;
            int text_y = m_y + (m_height + ref_metrics.height) / 2 - 2;

            ctx.drawText(text_x, text_y, m_title.c_str());

            int needed_w = metrics.width + 60;
            if (needed_w != m_cached_width)
            {
                m_cached_width = needed_w;
                set_fixed_size(m_cached_width);
            }
        }

    private:
        std::string m_title;
        int m_index;
        RibbonToolbar *m_toolbar;
        bool m_active = false;
        int m_cached_width = 0;
    };

    // --- RibbonSection ---
    RibbonSection::RibbonSection(const std::string &title) : Widget(), m_title(title)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(4);
        set_spacing(4);

        m_content_area = new Widget();
        m_content_area->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_content_area->set_spacing(6);
        m_content_area->set_position_type(FREE);
        add_child(std::unique_ptr<Widget>(m_content_area));
    }

    void RibbonSection::add_widget(std::unique_ptr<Widget> widget)
    {
        m_content_area->add_child(std::move(widget));
    }

    int RibbonSection::preferred_width() const
    {
        int w = m_margin * 2;
        int children_w = 0;
        for (auto &child : m_content_area->children())
        {
            children_w += child->preferred_width();
        }

        if (!m_content_area->children().empty())
        {
            children_w += m_content_area->spacing() * (m_content_area->children().size() - 1);
        }

        // Roughly estimate title width (8 pixels per char approx)
        int title_w = m_title.length() * 8 + 20;

        w += std::max(children_w, title_w);
        return w;
    }

    void RibbonSection::calculate_layout()
    {
        Widget::calculate_layout();

        int title_height = 20;

        m_content_area->set_position(x() + m_margin, y() + m_margin);
        m_content_area->set_size(width() - (m_margin * 2),
                                 height() - (m_margin * 2) - title_height);
        m_content_area->calculate_layout();
    }

    void RibbonSection::draw(GraphicsContext &ctx)
    {
        Color fg = theme_manager()->get_color("window_fg");
        auto font = theme_manager()->get_font("ui");

        ctx.setColor(fg);
        ctx.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        TextMetrics metrics = ctx.getTextMetrics(m_title.c_str(), font.family.c_str(), font.size,
                                                 FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        int text_x = m_start_draw_x + (m_width - metrics.width) / 2;
        int text_y = m_start_draw_y + m_height - m_margin - 4;

        ctx.drawText(text_x, text_y, m_title.c_str());

        Color line_color = theme_manager()->get_color("sidebar_fg");
        line_color.a = 0.3f;
        ctx.setColor(line_color);
        ctx.drawRect(m_start_draw_x + m_width - 1, m_start_draw_y + 10, 1, m_height - 20, 0, 1.0f);
    }

    // --- RibbonToolbar ---
    RibbonToolbar::RibbonToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        container->set_spacing(5);
        container->set_margin(5);
        container->set_background_color(theme_manager()->get_color("sidebar_bg"));

        auto frame_container = std::make_unique<Widget>();
        frame_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        frame_container->set_background_color(theme_manager()->get_color("window_bg"));
        frame_container->set_border_radius(6);
        // container->set_debug_mode(true);

        m_header = new Widget();
        m_header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_header->set_spacing(2);
        m_header->set_margin(0);
        m_header->set_fixed_size(20);
        add_child(std::unique_ptr<Widget>(m_header));

        m_content_frame = new Frame();
        m_content_frame->set_position_type(FILL);
        frame_container->add_child(std::unique_ptr<Widget>(m_content_frame));

        container->add_child(std::move(frame_container));

        add_child(std::move(container));

        // add_child(Spacer(5));

        when_mouse_wheel.connect([this](MouseWheelEventContext &ev) { handle_mouse_wheel(ev); });
    }

    int RibbonToolbar::add_tab(const std::string &title)
    {
        int index = m_tabs.size();

        auto button = std::make_unique<RibbonTabButton>(title, index, this);
        RibbonTabButton *btn_ptr = button.get();
        m_header->add_child(std::move(button));

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        container->set_spacing(10);
        container->set_margin(4);

        if (application())
        {
            container->set_application_recursive(application());
        }

        m_tabs.push_back({title, std::move(container), btn_ptr});

        if (m_active_tab_index == -1)
        {
            set_active_tab(0);
        }

        invalidate();
        return index;
    }

    RibbonSection *RibbonToolbar::add_section(int tab_index, const std::string &section_title)
    {
        if (tab_index < 0 || tab_index >= (int)m_tabs.size())
            return nullptr;

        auto section = std::make_unique<RibbonSection>(section_title);
        RibbonSection *ptr = section.get();

        m_tabs[tab_index].content_container->add_child(std::move(section));

        return ptr;
    }

    void RibbonToolbar::set_active_tab(int index)
    {
        if (index < 0 || index >= (int)m_tabs.size())
            return;

        if (m_active_tab_index != -1)
        {
            static_cast<RibbonTabButton *>(m_tabs[m_active_tab_index].button)->set_active(false);
        }

        m_active_tab_index = index;
        static_cast<RibbonTabButton *>(m_tabs[m_active_tab_index].button)->set_active(true);

        m_scroll_x = 0;
        invalidate();
    }

    void RibbonToolbar::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        for (auto &tab : m_tabs)
        {
            if (tab.content_container)
            {
                tab.content_container->set_application_recursive(app);
            }
        }
    }

    void RibbonToolbar::handle_mouse_wheel(MouseWheelEventContext &ev)
    {
        if (m_active_tab_index == -1)
            return;

        double delta = ev.dy * 40.0;
        if (std::abs(ev.dx) > 0)
        {
            delta = ev.dx * 40.0;
        }

        double old_scroll = m_scroll_x;
        m_scroll_x += delta;

        if (m_scroll_x < 0)
            m_scroll_x = 0;
        if (m_scroll_x > m_max_scroll_x)
            m_scroll_x = m_max_scroll_x;

        if (std::abs(m_scroll_x - old_scroll) > 0.001)
        {
            invalidate();
            ev.stop_propagation = true;
        }
    }

    void RibbonToolbar::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_active_tab_index != -1)
        {
            Widget *container = m_tabs[m_active_tab_index].content_container.get();

            int h = m_content_frame->height();

            container->set_position(m_content_frame->x(), m_content_frame->y());
            container->set_size(m_content_frame->width(), h);

            int needed_w = container->margin() * 2;

            for (auto &child : container->children())
            {
                int child_w = child->preferred_width();
                if (child_w < 50)
                    child_w = 100;

                child->set_fixed_size(child_w);

                needed_w += child_w + container->spacing();
            }

            if (!container->children().empty())
            {
                needed_w -= container->spacing();
            }

            m_max_scroll_x = std::max(0.0, (double)(needed_w - m_content_frame->width()));
            if (m_scroll_x > m_max_scroll_x)
                m_scroll_x = m_max_scroll_x;
        }
    }

    Widget *RibbonToolbar::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;
        if (x < m_x || y < m_y || x > m_x + m_width || y > m_y + m_height)
            return nullptr;

        if (Widget *hit = m_header->hit_test(x, y))
        {
            return hit;
        }

        if (m_active_tab_index != -1)
        {
            if (x >= m_content_frame->x() && x <= m_content_frame->x() + m_content_frame->width() &&
                y >= m_content_frame->y() && y <= m_content_frame->y() + m_content_frame->height())
            {
                int local_x = x + (int)m_scroll_x;
                Widget *container = m_tabs[m_active_tab_index].content_container.get();
                if (Widget *hit = container->hit_test(local_x, y))
                {
                    return hit;
                }
            }
        }

        return this;
    }

    void RibbonToolbar::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);

        if (!m_visible)
            return;

        bool intersects =
            !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);
        if (!intersects)
            return;

        bool should_draw = m_dirty || force || m_child_dirty;

        if (m_active_tab_index != -1)
        {
            gc.save();
            gc.clip(m_content_frame->x(), m_content_frame->y(), m_content_frame->width(),
                    m_content_frame->height());

            gc.save();
            gc.translate(-m_scroll_x, 0);

            Widget *container = m_tabs[m_active_tab_index].content_container.get();
            // Shift hit/clip region for children rendering
            container->render(gc, cx + (int)m_scroll_x, cy, cw, ch, should_draw);

            gc.restore();
            gc.restore();
        }
    }

    void RibbonToolbar::draw(GraphicsContext &ctx)
    {
        if (!theme_manager())
            return;

        Color title_bg = theme_manager()->get_color("ribbon_tab_title_bg");
        Color title_bg_dark = title_bg.darker(20.0f);
        int header_bottom = m_header->y() + m_header->height();
        int header_total_h = header_bottom - m_y;

        // Gradiente de fondo del header completo (desde el tope del toolbar)
        ctx.fillLinearGradientRect(m_x, m_y, m_width, header_total_h, title_bg, title_bg_dark,
                                   /*vertical=*/true);

        // Fondo del tab activo: más claro que ribbon_tab_active_title_bg base
        if (m_active_tab_index >= 0 && m_active_tab_index < (int)m_tabs.size())
        {
            Widget *btn = m_tabs[m_active_tab_index].button;
            Color active_bg =
                theme_manager()->get_color("ribbon_tab_active_title_bg").lighter(15.0f);
            ctx.setColor(active_bg);
            ctx.fillRect(btn->x(), m_y, btn->width(), header_total_h);
        }

        // Bordes verticales de los tabs (0.5f para que sean finos)
        Color brd = theme_manager()->get_color("window_brd");
        ctx.setColor(brd);
        for (int i = 0; i < (int)m_tabs.size(); ++i)
        {
            Widget *btn = m_tabs[i].button;
            int bx = btn->x();
            int bw = btn->width();

            // Borde derecho: se omite si el siguiente tab es el activo,
            // ya que el activo dibujará su propio borde izquierdo en ese borde
            bool next_is_active = (i + 1 == m_active_tab_index);
            if (!next_is_active)
            {
                ctx.drawLine(bx + bw - 1, m_y, bx + bw - 1, header_bottom, 0.5f);
            }

            // El tab activo además tiene borde izquierdo
            if (i == m_active_tab_index)
            {
                ctx.drawLine(bx, m_y, bx, header_bottom, 0.5f);
            }
        }
    }

} // namespace horizon
