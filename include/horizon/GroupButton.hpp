#pragma once

#include <horizon/Frame.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{

    class GroupButtonClickEvent : public EventContext
    {
    public:
        int button_index;
        std::string button_text;
    };

    class GroupButton : public Widget
    {
    public:
        GroupButton();
        ~GroupButton();

        void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void draw(GraphicsContext &ctx) override;

        void add_item(std::string text);
        void add_item(std::unique_ptr<Icon> icon);

        void set_current_item(int index);
        int current_item() const;

        EventsManager<GroupButtonClickEvent> when_button_clicked;

    private:
        void configure();
        int m_current_index{-1};
    };
} // namespace horizon
