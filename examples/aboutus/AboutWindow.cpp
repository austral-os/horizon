#include "AboutWindow.hpp"
#include "DiskInfo.hpp"
#include "Displays.hpp"
#include "Overview.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
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

        navigation->when_button_clicked.connect(
            [this](GroupButtonClickEvent &ev)
            {
                if (ev.button_text == "Overview")
                {
                    std::cout << "Overview" << std::endl;
                    set_content(std::make_unique<Overview>());
                }
                else if (ev.button_text == "Displays")
                {
                    std::cout << "Displays" << std::endl;
                    set_content(std::make_unique<Displays>());
                }
                else if (ev.button_text == "Storage")
                {
                    std::cout << "Storage" << std::endl;
                    set_content(std::make_unique<DiskInfoWidget>());
                }
                else if (ev.button_text == "Support")
                {
                    std::cout << "Support" << std::endl;
                }
                else if (ev.button_text == "Service")
                {
                    std::cout << "Service" << std::endl;
                }
            });

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);

        auto spacer2 = std::make_unique<Widget>();
        spacer2->set_position_type(FILL);

        tool_widget->add_child(std::move(spacer));
        tool_widget->add_child(std::move(navigation));
        tool_widget->add_child(std::move(spacer2));

        toolbar()->add_toolbar_widget(std::move(tool_widget));

        set_content(std::make_unique<Overview>());
    }

    void AboutWindow::set_content(std::unique_ptr<Widget> content)
    {
        if (children().size() > 1)
        {
            remove_child_at(1);
        }
        add_child(std::move(content));
    }
} // namespace horizon
