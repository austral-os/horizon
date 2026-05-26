#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/ToggleGroupButton.hpp>

namespace horizon
{
    ToggleGroupButton::ToggleGroupButton() : GroupButton() {}

    ToggleGroupButton::~ToggleGroupButton() {}

    void ToggleGroupButton::set_current_item(int index)
    {
        m_current_index = index;
        configure();
        invalidate();
    }

    int ToggleGroupButton::current_item() const
    {
        return m_current_index;
    }

    void ToggleGroupButton::configure()
    {
        GroupButton::configure();

        int index = 0;
        for (const auto &item : children())
        {
            if (auto button = dynamic_cast<Button<SolidObject> *>(item.get()))
            {
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

    void ToggleGroupButton::add_item(std::string text, int width)
    {
        int index = children().size();
        GroupButton::add_item(text, width);

        auto &btn_ptr = children().back();
        if (auto button = dynamic_cast<Button<SolidObject> *>(btn_ptr.get()))
        {
            button->when_mouse_press.connect([this, index](MouseButtonEventContext &ev)
                                             { set_current_item(index); });

            auto override_state = [this, button, index](EventContext &ev)
            {
                if (m_current_index == index)
                {
                    button->set_draw_state(WidgetDrawState::PRESSED);
                    button->invalidate();
                }
            };

            button->when_mouse_enter.connect(override_state);
            button->when_mouse_leave.connect(override_state);
            button->when_mouse_release.connect([override_state](MouseButtonEventContext &ev)
                                               { override_state(ev); });
        }
    }

    void ToggleGroupButton::add_item(std::unique_ptr<Icon> icon, int width)
    {
        int index = children().size();
        GroupButton::add_item(std::move(icon), width);

        auto &btn_ptr = children().back();
        if (auto button = dynamic_cast<Button<SolidObject> *>(btn_ptr.get()))
        {
            button->when_mouse_press.connect([this, index](MouseButtonEventContext &ev)
                                             { set_current_item(index); });

            auto override_state = [this, button, index](EventContext &ev)
            {
                if (m_current_index == index)
                {
                    button->set_draw_state(WidgetDrawState::PRESSED);
                    button->invalidate();
                }
            };

            button->when_mouse_enter.connect(override_state);
            button->when_mouse_leave.connect(override_state);
            button->when_mouse_release.connect([override_state](MouseButtonEventContext &ev)
                                               { override_state(ev); });
        }
    }
} // namespace horizon
