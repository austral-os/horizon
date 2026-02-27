#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using horizon::Application;
using horizon::AquaObject;
using horizon::Button;
using horizon::Notebook;
using horizon::NotebookPage;
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

        notebook->add_tab(NotebookPage("Tab 3", "Icon 3", std::move(container)));
        // Tab 1: Icon test
        auto icon_container = std::make_unique<Widget>();
        icon_container->set_margin(20);
        icon_container->set_spacing(10);

        auto icon1 = std::make_unique<horizon::Icon>();
        icon1->set_icon_name("folder");
        icon1->set_icon_size(48);
        icon1->set_fixed_size(60);

        auto icon2 = std::make_unique<horizon::Icon>();
        icon2->set_icon_name("firefox");
        icon2->set_icon_size(48);
        icon2->set_fixed_size(60);

        auto icon3 = std::make_unique<horizon::Icon>();
        icon3->set_icon_name("utilities-terminal");
        icon3->set_icon_size(48);
        icon3->set_fixed_size(60);

        icon_container->add_child(std::move(icon1));
        icon_container->add_child(std::move(icon2));
        icon_container->add_child(std::move(icon3));

        notebook->add_tab(NotebookPage("Icons", std::move(icon_container)));
        notebook->add_tab(NotebookPage("Tab 2", "Icon 2", std::make_unique<Widget>()));

        auto tb_container = std::make_unique<Widget>();
        tb_container->set_margin(40);
        auto textbox = std::make_unique<TextBox>();
        textbox->set_placeholder("Nombre");
        textbox->set_text("");
        tb_container->add_child(std::move(textbox));

        notebook->add_tab(NotebookPage("TextBox", std::move(tb_container)));

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
