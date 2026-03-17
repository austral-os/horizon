#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Label.hpp>
#include <iostream>

using namespace horizon;

int main()
{
    try
    {
        Application app("horizon.dialog_demo", 400, 300);
        app.set_name("Dialog Demo");

        auto wnd = std::make_unique<Window>("MessageDialog Tester");
        wnd->set_margin(20);
        wnd->set_spacing(10);
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto lbl_result = std::make_unique<Label>("Resultados aparecerán aquí");
        auto *lbl_result_ptr = lbl_result.get();

        auto btn_alert = std::make_unique<Button<AquaObject>>();
        btn_alert->set_text("Mostrar Alert (Info)");
        btn_alert->set_fixed_size(40);
        btn_alert->when_click.connect([&app](MouseButtonEventContext &) {
            app.alert("Este es un mensaje de información importante.", "Información", MessageType::Info);
        });

        auto btn_warn = std::make_unique<Button<AquaObject>>();
        btn_warn->set_text("Mostrar Alert (Warning)");
        btn_warn->set_fixed_size(40);
        btn_warn->set_accent_color(WidgetAccentColor::Warning);
        btn_warn->when_click.connect([&app](MouseButtonEventContext &) {
            app.alert("Cuidado: Esta acción puede tener consecuencias.", "Advertencia", MessageType::Warning);
        });

        auto btn_error = std::make_unique<Button<AquaObject>>();
        btn_error->set_text("Mostrar Alert (Error)");
        btn_error->set_fixed_size(40);
        btn_error->set_accent_color(WidgetAccentColor::Error);
        btn_error->when_click.connect([&app](MouseButtonEventContext &) {
            app.alert("Se ha producido un error crítico en el sistema.", "Error", MessageType::Error);
        });

        auto btn_confirm = std::make_unique<Button<AquaObject>>();
        btn_confirm->set_text("Probar Confirm");
        btn_confirm->set_fixed_size(40);
        btn_confirm->when_click.connect([&app, lbl_result_ptr](MouseButtonEventContext &) {
            bool result = app.confirm("¿Seguro que deseas proceder con esta operación?", "Confirmar");
            lbl_result_ptr->set_text(result ? "Resultado: Aceptado" : "Resultado: Cancelado");
        });

        wnd->add_child(std::move(btn_alert));
        wnd->add_child(std::move(btn_warn));
        wnd->add_child(std::move(btn_error));
        wnd->add_child(std::move(btn_confirm));
        wnd->add_child(std::move(lbl_result));

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
