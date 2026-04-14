#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/ColorSelector.hpp>
#include <horizon/Logger.hpp>
#include <iostream>

using namespace horizon;

class ColorDemoWindow : public Window {
public:
    ColorDemoWindow() : Window("Demo de Selección de Color") {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);
        set_spacing(20);

        // Preview label
        auto label = std::make_unique<Label>("Horizon UI Kit - Color Picker Demo");
        label->set_font_size(24);
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_alignment(TextAlignment::Center);
        m_label_ptr = label.get();
        add_child(std::move(label));

        // Subtitle
        auto sub = std::make_unique<Label>("Usa el selector para cambiar el color del texto");
        sub->set_alignment(TextAlignment::Center);
        add_child(std::move(sub));

        // Color Selector
        auto selector = std::make_unique<ColorSelector>();
        selector->set_color(Color(0.2f, 0.4f, 0.8f)); // Initial color
        m_label_ptr->set_text_color(Color(0.2f, 0.4f, 0.8f));

        selector->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &ctx) {
            LOG_INFO << "Color seleccionado: " << ctx.color.to_hex();
            m_label_ptr->set_text_color(ctx.color);
            this->invalidate();
        });

        add_child(std::move(selector));
    }

private:
    Label *m_label_ptr{nullptr};
};

int main(int argc, char** argv) {
    try {
        Application app("horizon.color_demo", 500, 300);
        app.set_name("Color Picker Demo");

        app.set_root(std::make_unique<ColorDemoWindow>());
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
