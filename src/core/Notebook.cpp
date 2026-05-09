#include "horizon/EventsManager.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{

    const int NOTEBOOK_HEADER_HEIGHT = 34;
    const int NOTEBOOK_BORDER_RADIUS = 14;

    Notebook::Notebook() : Widget()
    {

        set_position_type(WidgetPositionTypes::FILL);
        set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        set_margin(20);

        auto header = std::make_unique<Widget>();
        auto margin_top = std::make_unique<Widget>();
        auto header_sp1 = std::make_unique<Widget>();
        auto header_sp2 = std::make_unique<Widget>();

        auto body = std::make_unique<Frame>();

        header->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);

        body->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        body->set_position_type(WidgetPositionTypes::FILL);

        auto tab_spacer = std::make_unique<Widget>();
        tab_spacer->set_fixed_size(20); // 40px for header + 20px margin

        auto tab_content = std::make_unique<Widget>();
        tab_content->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        tab_content->set_position_type(WidgetPositionTypes::FILL);

        m_tab_content = tab_content.get();
        body->add_child(std::move(tab_spacer));
        body->add_child(std::move(tab_content));

        margin_top->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);

        header->set_position_type(WidgetPositionTypes::FREE);
        margin_top->set_fixed_size(20);

        header->add_child(std::move(header_sp1));
        header->add_child(std::move(header_sp2));

        m_header = header.get();
        m_margin_top = margin_top.get();
        m_body = body.get();

        add_child(std::move(margin_top));
        add_child(std::move(body)); // Body added before header so header is on top
        add_child(std::move(header));
    }

    Notebook::~Notebook() {}

    void Notebook::render(GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force)
    {
        m_header->set_position(m_x, m_y + m_margin_top->fixed_size());
        m_header->set_size(width(), NOTEBOOK_HEADER_HEIGHT);

        Widget::render(ctx, cx, cy, cw, ch, force);
    }

    void Notebook::draw(GraphicsContext &ctx)
    {
        Widget::draw(ctx);
    }

    void Notebook::configure_header()
    {
        int index = 0;
        int count_tabs = m_header->children().size() - 2;

        for (const auto &item : m_header->children())
        {

            // si item es de la clase Button<AquaObject>
            if (auto button = dynamic_cast<Button<AquaObject> *>(item.get()))
            {

                if (count_tabs > 1)
                {

                    if (index == 0)
                    {
                        button->set_corner_radius(
                            {NOTEBOOK_BORDER_RADIUS, 0, 0, NOTEBOOK_BORDER_RADIUS});
                    }
                    else if (index == count_tabs - 1)
                    {
                        button->set_corner_radius(
                            {0, NOTEBOOK_BORDER_RADIUS, NOTEBOOK_BORDER_RADIUS, 0});
                    }
                    else
                    {
                        button->set_corner_radius({0, 0, 0, 0});
                    }

                    button->set_accent_color(WidgetAccentColor::Default);

                    if (index == m_current_tab)
                    {
                        button->set_accent_color(WidgetAccentColor::Primary);
                    }
                }

                index++;
            }
        }
    }

    void Notebook::set_current_tab(int index)
    {

        if (index < 0 || index >= m_tab_content->children().size())
            return;

        if (m_current_tab == index)
            return;

        if (m_current_tab >= 0)
        {
            m_tab_content->children()[m_current_tab]->set_visible(false);
        }

        m_tab_content->children()[index]->set_visible(true);
        m_tab_content->children()[index]->invalidate();
        m_current_tab = index;

        configure_header();
        // Invalidate self and parent to ensure background is redrawn if transparent
        this->invalidate();
        if (parent())
            parent()->invalidate();
    }

    void Notebook::add_tab(NotebookPage page)
    {

        page.body->set_visible(false);

        m_tab_content->add_child(std::move(page.body));

        auto button = std::make_unique<Button<AquaObject>>();
        button->set_text(page.label);
        button->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);

        auto &children = m_header->children();

        int index = children.size() - 1;

        button->when_mouse_press.connect(
            [this, index](MouseButtonEventContext &context)
            {
                this->set_current_tab(index - 1);
                this->invalidate();
            });

        m_header->add_child_at(index, std::move(button));
        configure_header();

        if (m_current_tab < 0)
        {
            set_current_tab(0);
        }
    }

    void Notebook::add_tab(std::unique_ptr<NotebookPage> page)
    {

        page->body->set_visible(false);

        m_tab_content->add_child(std::move(page->body));

        auto button = std::make_unique<Button<AquaObject>>();
        button->set_text(page->label);
        button->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);

        auto &children = m_header->children();

        int index = children.size() - 1;

        button->when_mouse_press.connect(
            [this, index](MouseButtonEventContext &context)
            {
                this->set_current_tab(index - 1);
                this->invalidate();
            });

        m_header->add_child_at(index, std::move(button));
        configure_header();

        if (m_current_tab < 0)
        {
            set_current_tab(0);
        }
    }

} // namespace horizon
