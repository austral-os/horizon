#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    WaylandWindow app("combo.demo", 400, 300);

    auto wnd = std::make_unique<Window>("Combo Widget Demo");
    wnd->set_size(400, 300);

    auto main_container = std::make_unique<Widget>();
    main_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    main_container->set_margin(40);
    main_container->set_spacing(20);

    auto title = std::make_unique<Label>("Combo Widget Demo");
    title->set_font_size(18);
    title->set_font_weight(FONT_WEIGHT_BOLD);
    main_container->add_child(std::move(title));

    auto info_label = std::make_unique<Label>("Select an item:");
    main_container->add_child(std::move(info_label));

    auto combo = std::make_unique<Combo>();
    combo->add_item("item1", "Opción 1", "document-new");
    combo->add_item("item2", "Opción 2", "edit-copy");
    combo->add_item("item3", "Opción 3", "edit-paste");
    combo->add_item("item4", "Opción muy larga para probar el clipping", "help-about");

    auto *combo_ptr = combo.get();
    
    auto result_label = std::make_unique<Label>("Selected: None");
    auto *result_ptr = result_label.get();

    combo_ptr->when_item_selected.connect([result_ptr](ComboItemSelectedContext &ctx) {
        LOG_INFO << "Item selected: " << ctx.item.text << " (ID: " << ctx.item.id << ")";
        result_ptr->set_text("Selected: " + ctx.item.text);
    });

    main_container->add_child(std::move(combo));

    auto info_label2 = std::make_unique<Label>("Select an item (no icons):");
    main_container->add_child(std::move(info_label2));

    auto combo2 = std::make_unique<Combo>();
    combo2->add_item("c2_1", "Opcion A");
    combo2->add_item("c2_2", "Opcion B");
    combo2->add_item("c2_3", "Opcion C");
    
    auto *combo2_ptr = combo2.get();
    combo2_ptr->when_item_selected.connect([result_ptr](ComboItemSelectedContext &ctx) {
        LOG_INFO << "Combo 2 selected: " << ctx.item.text;
        result_ptr->set_text("Selected (Combo 2): " + ctx.item.text);
    });
    main_container->add_child(std::move(combo2));

    main_container->add_child(std::move(result_label));

    wnd->add_child(std::move(main_container));
    app.set_root(std::move(wnd));
    
    app.run();

    return 0;
}
