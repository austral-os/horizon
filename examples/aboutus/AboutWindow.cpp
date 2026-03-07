#include "AboutWindow.hpp"
#include "Overview.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    AboutWindow::AboutWindow() : ApplicationWindow("About System")
    {

        auto tool_widget = std::make_unique<Widget>();

        tool_widget->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        auto navigation = std::make_unique<horizon::GroupButton>();
        navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        navigation->set_fixed_size(600);

        navigation->add_item("Overview");
        navigation->add_item("Displays");
        navigation->add_item("Storage");
        navigation->add_item("Support");
        navigation->add_item("Service");

        auto content = std::make_unique<Overview>();

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);

        auto spacer2 = std::make_unique<Widget>();
        spacer2->set_position_type(FILL);

        tool_widget->add_child(std::move(spacer));
        tool_widget->add_child(std::move(navigation));
        tool_widget->add_child(std::move(spacer2));

        toolbar()->add_toolbar_widget(std::move(tool_widget));

        add_child(std::move(content));
    }
} // namespace horizon
