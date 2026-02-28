#pragma once

#include <horizon/Frame.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{

    class GroupButton : public Widget
    {
    public:
        GroupButton();
        ~GroupButton();

        void render(GraphicsContext &ctx) override;
        void draw(GraphicsContext &ctx) override;

        void add_item(std::string text);
        void add_item(std::unique_ptr<Icon> icon);

        void set_current_item(int index);

    private:
        void configure();
    };
} // namespace horizon
