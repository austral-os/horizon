#include <horizon/Notification.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <horizon/VPanel.hpp>
#include <memory>

using namespace horizon;

int main()
{
    try
    {
        WaylandWindow app("horizon.notification_demo", 800, 600);
        app.set_name("Notification Demo");

        auto wnd = std::make_unique<Window>("Notification Widget Showcase");
        wnd->set_size(800, 600);

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(40);
        container->set_spacing(20);
        container->set_background_color(Color{0.2f, 0.2f, 0.2f, 1.0f}); // Gray background to see transparency

        // 1. Standard notification with icon
        auto notif1 = std::make_unique<Notification>();
        notif1->set_notification("dialog-information", "Esta es una notificación estándar con un icono informativo.");
        container->add_child(std::move(notif1));

        // 2. Notification without icon (should take full width)
        auto notif2 = std::make_unique<Notification>();
        notif2->set_message("Esta respuesta no tiene icono, por lo que el texto ocupa todo el ancho disponible del widget.");
        container->add_child(std::move(notif2));

        // 3. Long text notification (should wrap and grow vertically)
        auto notif3 = std::make_unique<Notification>();
        notif3->set_notification("software-update-available", 
            "Hay una nueva actualización del sistema disponible para su descarga e instalación. "
            "Se recomienda encarecidamente instalarla para obtener las últimas mejoras de seguridad y rendimiento.");
        container->add_child(std::move(notif3));

        // 4. Fixed width notification
        auto notif4 = std::make_unique<Notification>();
        notif4->set_fixed_width(400);
        notif4->set_notification("dialog-warning", "Notificación con ancho fijo de 400px. El texto debería ajustarse a este espacio.");
        container->add_child(std::move(notif4));

        wnd->add_child(std::move(container));
        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception &e)
    {
        return 1;
    }
    return 0;
}
