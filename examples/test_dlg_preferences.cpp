#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Label.hpp>
#include <horizon/DialogPreferences.hpp>
#include <horizon/Icon.hpp>
#include <iostream>

using namespace horizon;

int main()
{
    try
    {
        Application app("horizon.test_dlg_preferences", 400, 300);
        app.set_name("Test DialogPreferences");

        auto wnd = std::make_unique<Window>("DialogPreferences Tester");
        wnd->set_margin(20);
        wnd->set_spacing(10);
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto lbl_info = std::make_unique<Label>("Click the button to open DialogPreferences");
        lbl_info->set_alignment(TextAlignment::Center);

        auto btn_open = std::make_unique<Button<AquaObject>>();
        btn_open->set_text("Open Preferences Dialog");
        btn_open->set_fixed_size(40);
        btn_open->when_click.connect([&app](MouseButtonEventContext &) {
            auto dialog = std::make_unique<DialogPreferences>("User Preferences", 500, 400);
            
            // Add something to the toolbar
            auto btn_save = std::make_unique<Button<AquaObject>>();
            btn_save->set_text("Save");
            btn_save->set_fixed_size(80);
            btn_save->set_accent_color(WidgetAccentColor::Primary);
            
            dialog->toolbar()->add_toolbar_widget(std::move(btn_save));

            // Set content
            auto content = std::make_unique<Widget>();
            content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            content->set_margin(20);
            content->set_spacing(10);

            auto lbl_content = std::make_unique<Label>("This is the preferences content area.");
            content->add_child(std::move(lbl_content));

            dialog->set_content(std::move(content));

            // Run dialog in a separate thread (Horizon pattern for dialogs)
            std::thread([d = std::move(dialog)]() mutable {
                d->initialize();
                d->run();
            }).detach();
        });

        wnd->add_child(std::move(lbl_info));
        wnd->add_child(std::move(btn_open));

        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
