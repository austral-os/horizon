#include "horizon/EventsManager.hpp"
#include <horizon/Button.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <utility>

namespace horizon
{

    GroupButton::GroupButton() : Widget()
    {

        set_position_type(WidgetPositionTypes::FILL);
        set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
    }

    GroupButton::~GroupButton() {}

    void GroupButton::render(GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force)
    {
        configure();

        Widget::render(ctx, cx, cy, cw, ch, force);
    }

    void GroupButton::draw(GraphicsContext &ctx)
    {
        Widget::draw(ctx);
    }

    void GroupButton::configure()
    {
        int index = 0;
        int items_count = children().size();
        for (const auto &item : children())
        {

            // si item es de la clase Button<AquaObject>
            if (auto button = dynamic_cast<Button<SolidObject> *>(item.get()))
            {

                if (items_count > 1)
                {

                    if (index == 0)
                    {
                        button->set_corner_radius({10, 0, 0, 10});
                    }
                    else if (index == items_count - 1)
                    {
                        button->set_corner_radius({0, 10, 10, 0});
                    }
                    else
                    {
                        button->set_corner_radius({0, 0, 0, 0});
                    }

                    button->set_accent_color(WidgetAccentColor::Default);
                }

                index++;
            }
        }
    }

    void GroupButton::add_item(std::string text, int width)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        button->set_text(text);
        if (width > 0)
        {
            button->set_fixed_size(width);
        }

        auto ptr_button = button.get();

        int index = children().size();

        button->when_mouse_press.connect(
            [this, text, index](MouseButtonEventContext &ev)
            {
                GroupButtonClickEvent event;
                event.button_index = index;
                event.button_text = text;
                when_button_clicked.run(event);
            });

        add_child(std::move(button));
    }

    void GroupButton::add_item(std::unique_ptr<Icon> icon, int width)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        icon->set_fixed_size(m_available_draw_height);
        icon->set_vertical_alignment(VerticalAlignment::Middle);
        button->add_child(std::move(icon));
        if (width > 0)
        {
            button->set_fixed_size(width);
        }
        auto ptr_button = button.get();

        int index = children().size();

        button->when_mouse_press.connect(
            [this, index](MouseButtonEventContext &ev)
            {
                GroupButtonClickEvent event;
                event.button_index = index;
                event.button_text = "";
                when_button_clicked.run(event);
            });

        add_child(std::move(button));
    }

} // namespace horizon
