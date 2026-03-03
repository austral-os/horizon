#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/ColorPicker.hpp>
#include <horizon/Frame.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/RadioButton.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Sidebar.hpp>
#include <horizon/SidebarItem.hpp>
#include <horizon/Slider.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TableView.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Textarea.hpp>
#include <horizon/VPanel.hpp>
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
        btn2->set_accent_color(WidgetAccentColor::Primary);

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
        auto textbox = std::make_unique<TextBox<horizon::TextPolicy>>();
        textbox->set_placeholder("Nombre");
        textbox->set_text("");
        tb_container->add_child(std::move(textbox));

        auto searchbox = std::make_unique<horizon::SearchBox>();
        searchbox->set_placeholder("Buscar...");
        tb_container->add_child(std::move(searchbox));

        auto pb = std::make_unique<ProgressBar>();
        pb->set_progress(0.65f);
        pb->set_margin(10);
        pb->set_fixed_size(30);
        tb_container->add_child(std::move(pb));

        // Slider 1: Horizontal, Marker shape, show ticks (Magnet)
        auto slider_h1 = std::make_unique<horizon::Slider>();
        slider_h1->set_value(0.35f);
        slider_h1->set_tick_count(5);
        slider_h1->set_show_ticks(true);
        slider_h1->set_thumb_shape(horizon::ThumbShape::Marker);
        slider_h1->set_margin(10);
        slider_h1->set_fixed_size(46);
        tb_container->add_child(std::move(slider_h1));

        // Slider 2: Horizontal, Circle shape, hide ticks (snapping still works)
        auto slider_h2 = std::make_unique<horizon::Slider>();
        slider_h2->set_value(0.75f);
        slider_h2->set_tick_count(11);
        slider_h2->set_show_ticks(false);
        slider_h2->set_thumb_shape(horizon::ThumbShape::Circle);
        slider_h2->set_margin(10);
        slider_h2->set_fixed_size(46);
        tb_container->add_child(std::move(slider_h2));

        auto icnFolder = std::make_unique<horizon::Icon>();
        icnFolder->set_icon_name("folder");
        icnFolder->set_icon_size(24);
        auto group_button = std::make_unique<horizon::GroupButton>();
        group_button->add_item("Item 1");
        group_button->add_item(std::move(icnFolder));
        group_button->add_item("Item 3");
        group_button->set_fixed_size(40);
        tb_container->add_child(std::move(group_button));

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

        // --- New Tab for ScrollArea demo ---
        auto scroll_area = std::make_unique<horizon::ScrollArea>();
        auto large_content = std::make_unique<Widget>();
        large_content->set_position_type(horizon::FREE);
        large_content->set_size(1200, 1000);

        auto scroll_lbl1 = std::make_unique<Label>("Scrolling test - Top Left");
        scroll_lbl1->set_position(20, 20);
        scroll_lbl1->set_size(200, 30);
        large_content->add_child(std::move(scroll_lbl1));

        auto scroll_lbl2 = std::make_unique<Label>("Bottom Right corner reached!");
        scroll_lbl2->set_position(1000, 950);
        scroll_lbl2->set_size(200, 30);
        large_content->add_child(std::move(scroll_lbl2));

        // Add a colored object for visibility
        auto color_rect = std::make_unique<horizon::SolidObject>();
        color_rect->set_position(400, 400);
        color_rect->set_size(400, 200);
        color_rect->set_accent_color(WidgetAccentColor::Primary);
        large_content->add_child(std::move(color_rect));

        scroll_area->set_content(std::move(large_content));
        notebook->add_tab(NotebookPage("Scroll", std::move(scroll_area)));

        // --- New Tab for TableView demo ---
        struct Person
        {
            int id;
            std::string name;
            std::string email;
            std::string role;
        };

        auto table_view = std::make_unique<horizon::TableView<Person>>();

        horizon::TableColumn<Person> col_id;
        col_id.id = "id";
        col_id.title = "ID";
        col_id.width = 60;
        col_id.cell_factory = [](const Person &p)
        { return std::make_unique<Label>(std::to_string(p.id)); };
        col_id.sort_predicate = [](const Person &a, const Person &b) { return a.id < b.id; };

        horizon::TableColumn<Person> col_name;
        col_name.id = "name";
        col_name.title = "Nombre";
        col_name.width = 200;
        col_name.cell_factory = [](const Person &p)
        {
            auto lbl = std::make_unique<Label>(p.name);
            lbl->set_font_weight(FONT_WEIGHT_BOLD);
            return lbl;
        };
        col_name.sort_predicate = [](const Person &a, const Person &b) { return a.name < b.name; };

        horizon::TableColumn<Person> col_email;
        col_email.id = "email";
        col_email.title = "Email";
        col_email.width = 250;
        col_email.cell_factory = [](const Person &p) { return std::make_unique<Label>(p.email); };
        col_email.sort_predicate = [](const Person &a, const Person &b)
        { return a.email < b.email; };

        horizon::TableColumn<Person> col_role;
        col_role.id = "role";
        col_role.title = "Rol";
        col_role.width = 120;
        col_role.cell_factory = [](const Person &p)
        {
            auto lbl = std::make_unique<Label>(p.role);
            if (p.role == "Admin")
                lbl->set_accent_color(WidgetAccentColor::Error);
            return lbl;
        };
        col_role.sort_predicate = [](const Person &a, const Person &b) { return a.role < b.role; };

        table_view->add_column(col_id);
        table_view->add_column(col_name);
        table_view->add_column(col_email);
        table_view->add_column(col_role);

        std::vector<Person> people;
        for (int i = 1; i <= 70; ++i)
        {
            people.push_back({i, "Usuario " + std::to_string(i),
                              "user" + std::to_string(i) + "@example.com",
                              (i % 5 == 0) ? "Admin" : "User"});
        }
        table_view->set_data(std::move(people));
        table_view->set_alternate_colors(horizon::Color(1.0f, 1.0f, 1.0f, 1.0f),
                                         horizon::Color(0.95f, 0.97f, 1.0f, 1.0f));
        table_view->set_width_mode(horizon::TableViewWidthMode::Unbounded);

        auto table_container = std::make_unique<Widget>();
        table_container->set_margin(30);
        table_container->add_child(std::move(table_view));

        notebook->add_tab(NotebookPage("Table", std::move(table_container)));

        // --- New Tab for ColorPicker demo ---
        auto color_container = std::make_unique<Widget>();
        color_container->set_margin(30);
        auto cp = std::make_unique<horizon::ColorPicker>();
        color_container->add_child(std::move(cp));
        notebook->add_tab(NotebookPage("Color", std::move(color_container)));

        // --- New Tab for VPanel demo ---
        auto vpanel_container = std::make_unique<Widget>();
        vpanel_container->set_margin(30);

        auto vpanel = std::make_unique<horizon::VPanel>();
        vpanel->set_spacing(10);

        auto sidebar = std::make_unique<horizon::Sidebar>();
        sidebar->add_group("Favorites");
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("user-home", "All My Files"));
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("folder-remote", "iCloud Drive"));
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("system-run", "Aplicaciones"));
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("user-desktop", "Desktop"));
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("folder-documents", "Documents"));
        sidebar->add_item("Favorites",
                          std::make_unique<horizon::SidebarItem>("folder-download", "Downloads"));

        sidebar->add_group("Devices");
        for (int i = 1; i <= 3; ++i)
        {
            sidebar->add_item("Devices", std::make_unique<horizon::SidebarItem>(
                                             "drive-harddisk", "Disk " + std::to_string(i)));
        }

        struct FileEntry
        {
            std::string name;
            std::string size;
            bool is_folder;
        };

        auto file_table = std::make_unique<horizon::TableView<FileEntry>>();
        file_table->set_width_mode(horizon::TableViewWidthMode::Fill);

        horizon::TableColumn<FileEntry> col_file_icon;
        col_file_icon.id = "icon";
        col_file_icon.title = "";
        col_file_icon.width = 32;
        col_file_icon.cell_factory = [](const FileEntry &f)
        {
            auto icon = std::make_unique<horizon::Icon>();
            icon->set_icon_name(f.is_folder ? "folder" : "text-x-generic");
            icon->set_icon_size(24);
            return icon;
        };
        col_file_icon.sort_predicate = [](const FileEntry &a, const FileEntry &b)
        { return a.is_folder && !b.is_folder; };

        horizon::TableColumn<FileEntry> col_file_name;
        col_file_name.id = "name";
        col_file_name.title = "Nombre";
        col_file_name.width = 300;
        col_file_name.cell_factory = [](const FileEntry &f)
        {
            auto lbl = std::make_unique<Label>(f.name);
            if (f.is_folder)
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
            return lbl;
        };
        col_file_name.sort_predicate = [](const FileEntry &a, const FileEntry &b)
        { return a.name < b.name; };

        horizon::TableColumn<FileEntry> col_file_size;
        col_file_size.id = "size";
        col_file_size.title = "Tamaño";
        col_file_size.width = 100;
        col_file_size.cell_factory = [](const FileEntry &f)
        { return std::make_unique<Label>(f.size); };
        col_file_size.sort_predicate = [](const FileEntry &a, const FileEntry &b)
        { return a.size < b.size; };

        file_table->add_column(col_file_icon);
        file_table->add_column(col_file_name);
        file_table->add_column(col_file_size);

        std::vector<FileEntry> files;
        files.push_back({"Documents", "---", true});
        files.push_back({"Downloads", "---", true});
        files.push_back({"Photos", "---", true});
        files.push_back({"report.pdf", "2.4 MB", false});
        files.push_back({"vacation.jpg", "4.1 MB", false});
        files.push_back({"notes.txt", "12 KB", false});
        files.push_back({"Work", "---", true});
        files.push_back({"Presentation.pptx", "15.0 MB", false});
        files.push_back({"Budget.xlsx", "850 KB", false});
        files.push_back({"Projects", "---", true});
        files.push_back({"script.py", "5 KB", false});
        files.push_back({"todo.md", "1 KB", false});
        files.push_back({"Movies", "---", true});
        files.push_back({"avatar.png", "250 KB", false});
        files.push_back({"backup.zip", "1.2 GB", false});
        files.push_back({"Source", "---", true});
        files.push_back({"main.cpp", "45 KB", false});
        files.push_back({"README", "2 KB", false});
        files.push_back({"Music", "---", true});
        files.push_back({"song.mp3", "8 MB", false});

        file_table->set_data(std::move(files));

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(file_table));

        vpanel_container->add_child(std::move(vpanel));
        notebook->add_tab(NotebookPage("VPanel", std::move(vpanel_container)));

        // --- New Tab for Textarea demo ---
        auto textarea_container = std::make_unique<Widget>();
        textarea_container->set_margin(30);
        textarea_container->set_spacing(15);

        auto textarea = std::make_unique<horizon::Textarea>();
        textarea->set_placeholder("Escribe aquí tu texto multilínea...");
        textarea->set_text("Este es un ejemplo de Textarea.\nSoporta múltiples líneas.\nPuedes "
                           "navegar con las flechas.\nIncluso moverte arriba y abajo!");

        textarea_container->add_child(std::move(textarea));
        notebook->add_tab(NotebookPage("Textarea", std::move(textarea_container)));

        notebook->set_current_tab(9);

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
