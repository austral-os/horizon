#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/WaylandWindow.hpp>

using namespace horizon;

class TransparentDemoWindow : public Window {
public:
    TransparentDemoWindow() : Window("Transparent Window Demo") {
        // Desactivamos el dibujo del fondo de la clase Window
        set_draw_background(false);
        
        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_margin(40);
        content->set_spacing(20);
        
        auto lbl = std::make_unique<Label>("Ventana Transparente");
        lbl->set_font_weight(FONT_WEIGHT_BOLD);
        lbl->set_fixed_size(30);
        
        auto lbl2 = std::make_unique<Label>("Si ves esto sobre el fondo de tu escritorio,\nla implementación es correcta.\n\nEl borde y la barra de título deben seguir siendo visibles.");
        
        content->add_child(std::move(lbl));
        content->add_child(std::move(lbl2));
        
        add_child(std::move(content));
    }
};

int main(int argc, char** argv) {
    // WaylandWindow proporciona la superficie RGBA y el bucle de eventos
    auto app = std::make_unique<WaylandWindow>("horizon.demo.transparent", 500, 400);
    
    // Creamos nuestra ventana que no dibuja el fondo
    auto win = std::make_unique<TransparentDemoWindow>();
    
    app->set_root(std::move(win));
    app->run();
    
    return 0;
}
