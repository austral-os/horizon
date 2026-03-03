#pragma once

#include <horizon/Titlebar.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{
    class Toolbar : public Titlebar
    {
    public:
        Toolbar(std::string title);
        ~Toolbar() = default;

        /**
         * @brief Adds a widget to the lower decorative area of the toolbar.
         */
        void add_toolbar_widget(std::unique_ptr<Widget> widget);

        void draw(GraphicsContext &gc) override;

    private:
        Widget *m_top_row{nullptr};
        Widget *m_bottom_row{nullptr};
    };
} // namespace horizon
