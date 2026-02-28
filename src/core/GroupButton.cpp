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

    void GroupButton::render(GraphicsContext &ctx)
    {
        configure();

        Widget::render(ctx);
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

    void GroupButton::add_item(std::string text)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        button->set_text(text);

        add_child(std::move(button));
    }

    void GroupButton::add_item(std::unique_ptr<Icon> icon)
    {

        auto button = std::make_unique<Button<SolidObject>>();
        button->add_child(std::move(icon));

        add_child(std::move(button));
    }

} // namespace horizon
