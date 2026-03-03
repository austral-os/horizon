#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    Application app(800, 600);

    auto window = std::make_unique<ApplicationWindow>("Horizon Toolbar Demo");
    window->set_size(800, 600);

    // Add some widgets to the toolbar
    auto file_btn = std::make_unique<Button<SolidObject>>();
    file_btn->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    file_btn->set_margin(4);
    file_btn->set_spacing(4);
    auto file_icon = std::make_unique<Icon>();
    file_icon->set_icon_name("folder-open");
    file_icon->set_icon_size(16);
    file_icon->set_fixed_size(16);
    file_btn->add_child(std::move(file_icon));
    file_btn->set_text("File");
    file_btn->set_fixed_size(80);

    auto edit_btn = std::make_unique<Button<SolidObject>>();
    edit_btn->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    edit_btn->set_margin(4);
    edit_btn->set_spacing(4);
    auto edit_icon = std::make_unique<Icon>();
    edit_icon->set_icon_name("edit");
    edit_icon->set_icon_size(16);
    edit_icon->set_fixed_size(16);
    edit_btn->add_child(std::move(edit_icon));
    edit_btn->set_text("Edit");
    edit_btn->set_fixed_size(80);

    auto help_btn = std::make_unique<Button<SolidObject>>();
    help_btn->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    help_btn->set_margin(4);
    help_btn->set_spacing(4);
    auto help_icon = std::make_unique<Icon>();
    help_icon->set_icon_name("help");
    help_icon->set_icon_size(16);
    help_icon->set_fixed_size(16);
    help_btn->add_child(std::move(help_icon));
    help_btn->set_text("Help");
    help_btn->set_fixed_size(80);

    window->toolbar()->add_toolbar_widget(std::move(file_btn));
    window->toolbar()->add_toolbar_widget(std::move(edit_btn));
    window->toolbar()->add_toolbar_widget(std::move(help_btn));

    // Content area (Window is Vertical, first child is Toolbar)
    auto content = std::make_unique<Widget>();
    content->set_position_type(FILL);
    content->add_child(std::make_unique<Label>("Main Content Area"));

    window->add_child(std::move(content));

    app.set_root(std::move(window));

    app.run();
    return 0;
}
