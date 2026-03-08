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

                // Ensure the selected button is in the PRESSED state if configured during
                // layout/render
                if (index == m_current_index)
                {
                    button->set_draw_state(WidgetDrawState::PRESSED);
                }
                else
                {
                    button->set_draw_state(WidgetDrawState::NORMAL);
                }

                index++;
            }
        }
    }

    void GroupButton::set_current_item(int index)
    {
        m_current_index = index;
        configure();
        invalidate();
    }

    int GroupButton::current_item() const
    {
        return m_current_index;
    }

    void GroupButton::add_item(std::string text)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        button->set_text(text);
        auto ptr_button = button.get();

        int index = children().size();

        button->when_mouse_press.connect(
            [this, text, index](MouseButtonEventContext &ev)
            {
                GroupButtonClickEvent event;
                event.button_index = index;
                event.button_text = text;
                when_button_clicked.run(event);

                set_current_item(index);
            });

        // Event handlers to override state transitions for the selected button
        auto override_state = [this, ptr_button, index](EventContext &ev)
        {
            if (m_current_index == index)
            {
                ptr_button->set_draw_state(WidgetDrawState::PRESSED);
                ptr_button->invalidate();
            }
        };

        button->when_mouse_enter.connect(override_state);
        button->when_mouse_leave.connect(override_state);
        button->when_mouse_release.connect([override_state](MouseButtonEventContext &ev)
                                           { override_state(ev); });

        add_child(std::move(button));
    }

    void GroupButton::add_item(std::unique_ptr<Icon> icon)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        icon->set_fixed_size(m_available_draw_height);
        button->add_child(std::move(icon));
        auto ptr_button = button.get();

        int index = children().size();

        button->when_mouse_press.connect(
            [this, index](MouseButtonEventContext &ev)
            {
                GroupButtonClickEvent event;
                event.button_index = index;
                event.button_text = "";
                when_button_clicked.run(event);

                set_current_item(index);
            });

        // Event handlers to override state transitions for the selected button
        auto override_state = [this, ptr_button, index](EventContext &ev)
        {
            if (m_current_index == index)
            {
                ptr_button->set_draw_state(WidgetDrawState::PRESSED);
                ptr_button->invalidate();
            }
        };

        button->when_mouse_enter.connect(override_state);
        button->when_mouse_leave.connect(override_state);
        button->when_mouse_release.connect([override_state](MouseButtonEventContext &ev)
                                           { override_state(ev); });

        add_child(std::move(button));
    }

} // namespace horizon
