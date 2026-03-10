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

        virtual void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                            bool force = false) override;
        virtual void draw(GraphicsContext &ctx) override;

        virtual void add_item(std::string text);
        virtual void add_item(std::unique_ptr<Icon> icon);

        EventsManager<GroupButtonClickEvent> when_button_clicked;

    protected:
        virtual void configure();

    private:
    };
} // namespace horizon
