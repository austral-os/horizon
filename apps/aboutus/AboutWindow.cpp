#include "AboutWindow.hpp"
#include <horizon/DiskInfoWidget.hpp>
#include "Displays.hpp"
#include "MemoryInfo.hpp"
#include <horizon/Overview.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/ToggleGroupButton.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <memory>
#include <horizon/I18n.hpp>

namespace horizon
{
    AboutWindow::AboutWindow() : ApplicationWindow(i18n().tr("aboutus.title"))
    {

        auto tool_widget = std::make_unique<Widget>();

        tool_widget->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        auto navigation = std::make_unique<horizon::ToggleGroupButton>();
        navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        navigation->set_fixed_size(600);

        navigation->add_item(i18n().tr("aboutus.tabs.overview"));
        navigation->add_item(i18n().tr("aboutus.tabs.displays"));
        navigation->add_item(i18n().tr("aboutus.tabs.storage"));
        navigation->add_item(i18n().tr("aboutus.tabs.memory"));

        navigation->set_current_item(0);

        navigation->when_button_clicked.connect(
            [this](GroupButtonClickEvent &ev)
            {
                if (ev.button_text == i18n().tr("aboutus.tabs.overview"))
                {

                    set_content(std::make_unique<Overview>());
                }
                else if (ev.button_text == i18n().tr("aboutus.tabs.displays"))
                {

                    set_content(std::make_unique<Displays>());
                }
                else if (ev.button_text == i18n().tr("aboutus.tabs.storage"))
                {

                    set_content(std::make_unique<DiskInfoWidget>(SystemInfo::get_os_disk_info()));
                }

                else if (ev.button_text == i18n().tr("aboutus.tabs.memory"))
                {
                    set_content(std::make_unique<MemoryInfoWidget>());
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

        show_status_bar();
        set_status_text(i18n().tr("aboutus.status"));

        set_content(std::make_unique<Overview>());
    }

    void AboutWindow::set_content(std::unique_ptr<Widget> content)
    {
        ApplicationWindow::set_content(std::move(content));
    }
} // namespace horizon
