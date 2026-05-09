#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Button.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/NotificationSender.hpp>
#include <iostream>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        Application app("org.horizon.notification_demo", 400, 300);
        
        app.about_manager().set_app_title("Notification Demo");
        app.about_manager().set_app_description("Demo for Horizon notification system");
        app.about_manager().set_app_version("1.0.0");
        app.about_manager().set_app_icon("dialog-information");
        
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_spacing(20);
        root->set_margin(30);

        auto btn_simple = std::make_unique<Button<SolidObject>>();
        btn_simple->set_text("Enviar Notificación Simple");
        btn_simple->when_click.connect([](MouseButtonEventContext &) {
            NotificationSender::send("Hola!", "Esta es una notificación de prueba desde la GUI.", "dialog-information");
        });

        auto btn_timed = std::make_unique<Button<SolidObject>>();
        btn_timed->set_text("Notificación de 2 Segundos");
        btn_timed->when_click.connect([](MouseButtonEventContext &) {
            NotificationSender::send("Recordatorio", "Esta notificación desaparecerá rápido.", "appointment-soon", 2000);
        });

        auto btn_multiple = std::make_unique<Button<SolidObject>>();
        btn_multiple->set_text("Enviar Ráfaga (3 Notificaciones)");
        btn_multiple->when_click.connect([](MouseButtonEventContext &) {
            NotificationSender::send("Mensaje 1", "Primero de la ráfaga.", "mail-unread");
            NotificationSender::send("Mensaje 2", "Segundo de la ráfaga.", "mail-unread");
            NotificationSender::send("Mensaje 3", "Tercero de la ráfaga.", "mail-unread");
        });

        root->add_child(std::move(btn_simple));
        root->add_child(std::move(btn_timed));
        root->add_child(std::move(btn_multiple));

        app.set_root(std::move(root));
        app.set_name("Notification GUI Demo");
        
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
