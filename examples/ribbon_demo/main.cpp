#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Label.hpp>
#include <horizon/RibbonToolbar.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    WaylandWindow app("horizon.ribbon_demo", 800, 600);
    app.set_name("Ribbon Demo");

    auto wnd = std::make_unique<Window>("Ribbon Demo");
    wnd->set_size(800, 600);

    auto container = std::make_unique<Widget>();
    container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    container->set_position_type(FILL);

    auto ribbon = std::make_unique<RibbonToolbar>();
    ribbon->set_fixed_size(130);

    // Tab 1: Home
    int t1 = ribbon->add_tab("Home");
    auto s1 = ribbon->add_section(t1, "Clipboard");

    auto b1 = std::make_unique<Button<AquaObject>>();
    b1->set_text("Paste");
    b1->set_fixed_size(60);
    s1->add_widget(std::move(b1));

    auto b2 = std::make_unique<Button<AquaObject>>();
    b2->set_text("Cut");
    b2->set_fixed_size(60);
    s1->add_widget(std::move(b2));

    auto b3 = std::make_unique<Button<AquaObject>>();
    b3->set_text("Copy");
    b3->set_fixed_size(60);
    s1->add_widget(std::move(b3));

    auto s2 = ribbon->add_section(t1, "Font");

    auto font_vbox = std::make_unique<Widget>();
    font_vbox->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    font_vbox->set_spacing(4);
    font_vbox->set_fixed_size(280);

    // Row 1
    auto row1 = std::make_unique<Widget>();
    row1->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    row1->set_spacing(4);

    auto combo_font = std::make_unique<Combo>();
    combo_font->add_item("1", "Arial");
    combo_font->add_item("2", "Times New Roman");
    combo_font->add_item("3", "Calibri");
    combo_font->set_selected_item_index(2);
    combo_font->set_fixed_size(140);

    auto combo_size = std::make_unique<Combo>();
    combo_size->add_item("1", "10");
    combo_size->add_item("2", "11");
    combo_size->add_item("3", "12");
    combo_size->add_item("4", "14");
    combo_size->set_selected_item_index(1);
    combo_size->set_fixed_size(60);

    auto btn_inc_dec = std::make_unique<GroupButton>();
    btn_inc_dec->add_item("A+");
    btn_inc_dec->add_item("A-");
    btn_inc_dec->set_fixed_size(60);

    row1->add_child(std::move(combo_font));
    row1->add_child(std::move(combo_size));
    row1->add_child(std::move(btn_inc_dec));

    // Row 2
    auto row2 = std::make_unique<Widget>();
    row2->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    row2->set_spacing(4);

    auto btn_styles = std::make_unique<GroupButton>();
    btn_styles->add_item("B");
    btn_styles->add_item("I");
    btn_styles->add_item("U");
    btn_styles->add_item("x₂");
    btn_styles->add_item("x²");
    btn_styles->set_fixed_size(150);

    auto btn_colors = std::make_unique<GroupButton>();
    btn_colors->add_item("ab");
    btn_colors->add_item("A_");
    btn_colors->set_fixed_size(60);

    row2->add_child(std::move(btn_styles));
    row2->add_child(std::move(btn_colors));

    font_vbox->add_child(std::move(row1));
    font_vbox->add_child(std::move(row2));

    s2->add_widget(std::move(font_vbox));

    // Tab 2: Layout
    int t2 = ribbon->add_tab("Layout");
    auto s4 = ribbon->add_section(t2, "Page Setup");
    auto b5 = std::make_unique<Button<AquaObject>>();
    b5->set_text("Margins");
    b5->set_fixed_size(60);
    s4->add_widget(std::move(b5));

    // Tab 3: View
    int t3 = ribbon->add_tab("View");
    auto s5 = ribbon->add_section(t3, "Zoom");
    auto b6 = std::make_unique<Button<AquaObject>>();
    b6->set_text("100%");
    b6->set_fixed_size(60);
    s5->add_widget(std::move(b6));

    container->add_child(std::move(ribbon));

    auto content = std::make_unique<SolidObject>();
    content->set_position_type(FILL);
    content->set_background_color(Color(0.9f, 0.9f, 0.9f, 1.0f));
    container->add_child(std::move(content));

    wnd->add_child(std::move(container));
    app.set_root(std::move(wnd));

    app.run();
    return 0;
}
