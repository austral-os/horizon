#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/DialogPreferences.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
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
        btn_open->when_click.connect(
            [&app](MouseButtonEventContext &)
            {
                // Instantiate DialogPreferences (no config required)
                auto dialog =
                    std::make_unique<DialogPreferences>("User Preferences", 500, 400, true);

                // Create PreferencesContent with an absolute path
                std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
                std::string absolute_path = home + "/" + "test_preferences_abs.json";

                auto pref_content = std::make_unique<PreferencesContent>(absolute_path);
                auto pref_content_ptr = pref_content.get();

                // Set some default configuration values on the content
                pref_content->set_config_value("general", "propiedad1", "valor");
                pref_content->set_config_value("advanced", "otra_propiedad", "valor");

                // Save immediately to verify the file creation
                pref_content->save_config();
                std::cout << "[Test] Configuration saved to " << absolute_path << std::endl;

                // Section 1: General
                auto section1 = std::make_unique<Widget>();
                section1->set_layout_type(WIDGET_LAYOUT_VERTICAL);
                section1->set_margin(20);
                section1->set_spacing(15);
                section1->add_child(std::make_unique<Label>("This is the General section"));

                pref_content->add_section("General", "preferences-system", std::move(section1));

                // Section 2: Advanced
                auto section2 = std::make_unique<Widget>();
                section2->set_layout_type(WIDGET_LAYOUT_VERTICAL);
                section2->set_margin(20);
                section2->set_spacing(15);
                section2->add_child(std::make_unique<Label>("This is the Advanced section"));

                pref_content->add_section("Advanced", "preferences-system-details",
                                          std::move(section2));

                // Setup the toolbar automatically using our new widget
                dialog->setup_toolbar(pref_content_ptr);

                dialog->set_content(std::move(pref_content));

                // Run dialog in a separate thread
                std::thread(
                    [d = std::move(dialog)]() mutable
                    {
                        d->initialize();
                        d->run();
                    })
                    .detach();
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
