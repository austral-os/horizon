#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/RadioButton.hpp>
#include <horizon/Slider.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using horizon::Application;
using horizon::AquaObject;
using horizon::Button;
using horizon::Checkbox;
using horizon::FONT_SLANT_ITALIC;
using horizon::FONT_WEIGHT_BOLD;
using horizon::Label;
using horizon::Notebook;
using horizon::NotebookPage;
using horizon::ProgressBar;
using horizon::RadioButton;
using horizon::Slider;
using horizon::SolidObject;
using horizon::TextAlignment;
using horizon::TextBox;
using horizon::Widget;
using horizon::WidgetAccentColor;
using horizon::Window;

int main()
{
    try
    {
        Application app(800, 600);

        auto wnd = std::make_unique<Window>("Horizon Application toolkit demo");
        wnd->set_size(800, 600);

        auto notebook = std::make_unique<Notebook>();

        auto container = std::make_unique<Widget>();
        container->set_margin(10);
        container->set_spacing(10);

        auto spacer1 = std::make_unique<Widget>();
        auto spacer2 = std::make_unique<Widget>();

        auto btn = std::make_unique<Button<AquaObject>>();
        auto btn2 = std::make_unique<Button<horizon::SolidObject>>();
        auto btn3 = std::make_unique<Button<AquaObject>>();
        auto btn4 = std::make_unique<Button<AquaObject>>();
        auto btn5 = std::make_unique<Button<AquaObject>>();
        auto btn6 = std::make_unique<Button<AquaObject>>();

        btn->set_text("Aceptar");
        btn->set_corner_radius({0, 20, 20, 0});
        btn->set_fixed_size(40);
        btn->set_accent_color(WidgetAccentColor::Primary);

        btn2->set_text("Cancelar");
        // btn2->set_corner_radius({0, 20, 20, 0});
        btn2->set_fixed_size(40);
        btn2->set_accent_color(WidgetAccentColor::Default);

        btn3->set_text("Cancelar");
        btn3->set_fixed_size(40);
        btn3->set_accent_color(WidgetAccentColor::Success);

        btn4->set_text("Cancelar");
        btn4->set_fixed_size(40);
        btn4->set_accent_color(WidgetAccentColor::Error);

        btn5->set_text("Cancelar");
        btn5->set_fixed_size(40);
        btn5->set_accent_color(WidgetAccentColor::Info);

        btn6->set_text("Cancelar");
        btn6->set_fixed_size(40);
        btn6->set_accent_color(WidgetAccentColor::Warning);

        container->add_child(std::move(spacer1));
        container->add_child(std::move(btn));
        container->add_child(std::move(btn2));
        container->add_child(std::move(btn3));
        container->add_child(std::move(btn4));
        container->add_child(std::move(btn5));
        container->add_child(std::move(btn6));
        container->add_child(std::move(spacer2));

        notebook->add_tab(NotebookPage("Buttons", "", std::move(container)));
        // Tab 1: Icon test
        auto icon_container = std::make_unique<Widget>();
        icon_container->set_margin(20);
        icon_container->set_spacing(10);

        auto icon1 = std::make_unique<horizon::Icon>();
        icon1->set_icon_name("folder");
        icon1->set_icon_size(48);

        auto icon2 = std::make_unique<horizon::Icon>();
        icon2->set_icon_name("kwrite");
        icon2->set_icon_size(128);

        auto icon3 = std::make_unique<horizon::Icon>();
        icon3->set_icon_name("utilities-terminal");
        icon3->set_icon_size(64);

        icon_container->add_child(std::move(icon1));
        icon_container->add_child(std::move(icon2));
        icon_container->add_child(std::move(icon3));

        notebook->add_tab(NotebookPage("Icons", std::move(icon_container)));

        auto tb_container = std::make_unique<Widget>();
        tb_container->set_margin(40);
        tb_container->set_spacing(10);
        auto textbox = std::make_unique<TextBox>();
        textbox->set_placeholder("Nombre");
        textbox->set_text("");
        tb_container->add_child(std::move(textbox));

        auto pb = std::make_unique<ProgressBar>();
        pb->set_progress(0.65f);
        pb->set_margin(10);
        pb->set_fixed_size(30);
        tb_container->add_child(std::move(pb));

        // Horizontal slider with tick marks
        auto slider_h = std::make_unique<horizon::Slider>();
        slider_h->set_value(0.35f);
        slider_h->set_tick_count(5);
        slider_h->set_margin(10);
        // slider_h->set_fixed_size(46);
        slider_h->set_orientation(horizon::SliderOrientation::Vertical);
        tb_container->add_child(std::move(slider_h));

        notebook->add_tab(NotebookPage("TextBox", std::move(tb_container)));

        // --- New Tab for Label demo ---
        auto label_container = std::make_unique<Widget>();
        label_container->set_margin(30);
        label_container->set_spacing(10);

        auto lbl1 = std::make_unique<Label>("Este es un Label normal con alineacion por defecto.");

        auto lbl2 = std::make_unique<Label>("Este es un Label en negrita y centrado.");
        lbl2->set_alignment(TextAlignment::Center);
        lbl2->set_font_weight(FONT_WEIGHT_BOLD);

        auto lbl3 = std::make_unique<Label>(
            "Este es un Label en cursiva y alineado a la derecha con un texto bastante largo para "
            "probar el ajuste de linea automatico (word wrap) que implementamos.");
        lbl3->set_alignment(TextAlignment::Right);
        lbl3->set_font_slant(FONT_SLANT_ITALIC);

        auto lbl4 = std::make_unique<Label>(
            "Este texto es extremadamente largo y deberia ser truncado con puntos suspensivos "
            "porque el widget tiene una altura fija que solo permite dos o tres lineas de texto. "
            "Estamos probando que la logica de calculate_lines detecte el exceso de altura y "
            "añada los puntos correspondientes al final de la ultima linea visible.");
        lbl4->set_fixed_size(40); // Restrict height to ~2 lines (size=14, spacing=4)

        label_container->add_child(std::move(lbl1));
        label_container->add_child(std::move(lbl2));
        label_container->add_child(std::move(lbl3));
        label_container->add_child(std::move(lbl4));

        notebook->add_tab(NotebookPage("Label", std::move(label_container)));

        // --- New Tab for Checkboxes & Radios ---
        auto widgets_container = std::make_unique<Widget>();
        widgets_container->set_margin(20);
        widgets_container->set_spacing(10);

        auto cb_aqua = std::make_unique<Checkbox<AquaObject>>();
        cb_aqua->set_text("Aqua Checkbox");
        cb_aqua->set_checked(true);

        auto cb_solid = std::make_unique<Checkbox<SolidObject>>();
        cb_solid->set_text(
            "Solid Checkbox con un texto extremadamente largo diseñado específicamente para probar "
            "que el nuevo sistema de Label interno funciona correctamente y divide el texto en "
            "varias líneas si es necesario.");

        auto rb_aqua1 = std::make_unique<RadioButton<AquaObject>>();
        rb_aqua1->set_text("Aqua Radio 1");
        rb_aqua1->set_selected(true);

        auto rb_aqua2 = std::make_unique<RadioButton<AquaObject>>();
        rb_aqua2->set_text(
            "Aqua Radio 2 con un texto que también debería ajustarse automáticamente si el espacio "
            "horizontal es insuficiente para mostrarlo en una sola línea.");

        auto rb_aqua1_ptr = rb_aqua1.get();
        auto rb_aqua2_ptr = rb_aqua2.get();

        // Simple manual grouping logic
        rb_aqua1_ptr->set_on_select([rb_aqua2_ptr]() { rb_aqua2_ptr->set_selected(false); });
        rb_aqua2_ptr->set_on_select([rb_aqua1_ptr]() { rb_aqua1_ptr->set_selected(false); });

        widgets_container->add_child(std::move(cb_aqua));
        widgets_container->add_child(std::move(cb_solid));
        widgets_container->add_child(std::move(rb_aqua1));
        widgets_container->add_child(std::move(rb_aqua2));

        notebook->add_tab(NotebookPage("Check/Radio", std::move(widgets_container)));

        wnd->add_child(std::move(notebook));

        app.set_root(std::move(wnd));
        app.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
