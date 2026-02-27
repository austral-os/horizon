#include "horizon/EventsManager.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <memory>

namespace horizon
{

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
        body->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
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

    void Notebook::render(GraphicsContext &ctx)
    {
        configure_header();

        m_header->set_position(m_x, m_y + m_margin_top->fixed_size());
        m_header->set_size(width(), 40);

        Widget::render(ctx);

        if (m_current_tab < 0 && m_body->children().size() > 0)
        {
            set_current_tab(0);
        }
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
                        button->set_corner_radius({10, 0, 0, 10});
                    }
                    else if (index == count_tabs - 1)
                    {
                        button->set_corner_radius({0, 10, 10, 0});
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

        if (index < 0 || index >= m_body->children().size())
            return;

        if (m_current_tab == index)
            return;

        if (m_current_tab >= 0)
        {
            m_body->children()[m_current_tab]->set_visible(false);
        }

        m_body->children()[index]->set_visible(true);
        m_body->children()[index]->invalidate();
        m_current_tab = index;

        // Invalidate self and parent to ensure background is redrawn if transparent
        this->invalidate();
        if (parent())
            parent()->invalidate();
    }

    void Notebook::add_tab(NotebookPage page)
    {

        page.body->set_visible(false);

        m_body->add_child(std::move(page.body));

        auto button = std::make_unique<Button<AquaObject>>();
        button->set_text(page.label);
        button->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);

        auto &children = m_header->children();

        int index = children.size() - 1;

        button->when_mouse_press.connect(
            [this, index](EventContext &context)
            {
                this->set_current_tab(index - 1);
                this->invalidate();
            });

        m_header->add_child_at(index, std::move(button));
    }

} // namespace horizon
