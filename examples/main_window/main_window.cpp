#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Button.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Sidebar.hpp>
#include <horizon/SidebarItem.hpp>
#include <horizon/TableView.hpp>
#include <horizon/VPanel.hpp>

#include <memory>

using namespace horizon;

class ArkToolbar : public Widget
{
public:
    ArkToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(5);

        auto navigation = std::make_unique<horizon::GroupButton>();
        navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        navigation->set_fixed_size(80);

        auto mini_spacer1 = std::make_unique<Widget>();
        mini_spacer1->set_fixed_size(20);

        auto back_icon = std::make_unique<horizon::Icon>();
        back_icon->set_icon_name("go-previous");
        back_icon->set_icon_size(16);
        navigation->add_item(std::move(back_icon));

        auto forward_icon = std::make_unique<horizon::Icon>();
        forward_icon->set_icon_name("go-next");
        forward_icon->set_icon_size(16);
        navigation->add_item(std::move(forward_icon));

        auto view_modes = std::make_unique<horizon::GroupButton>();
        view_modes->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        view_modes->set_fixed_size(140);
        view_modes->set_margin(0);

        auto icon_view = std::make_unique<horizon::Icon>();
        icon_view->set_icon_name("view-grid");
        icon_view->set_icon_size(16);
        view_modes->add_item(std::move(icon_view));

        auto list_view = std::make_unique<horizon::Icon>();
        list_view->set_icon_name("view-list");
        list_view->set_icon_size(16);
        view_modes->add_item(std::move(list_view));

        auto column_view = std::make_unique<horizon::Icon>();
        column_view->set_icon_name("view-column");
        column_view->set_icon_size(16);
        view_modes->add_item(std::move(column_view));

        auto cover_flow = std::make_unique<horizon::Icon>();
        cover_flow->set_icon_name("folder-pictures"); // Generic enough for cover flow demo
        cover_flow->set_icon_size(16);
        view_modes->add_item(std::move(cover_flow));

        auto search_box = std::make_unique<horizon::SearchBox>();
        search_box->set_placeholder("Buscar...");
        search_box->set_fixed_size(200);

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);

        add_child(std::move(navigation));
        add_child(std::move(mini_spacer1));
        add_child(std::move(view_modes));
        add_child(std::move(spacer));
        add_child(std::move(search_box));
    }
    ~ArkToolbar() = default;
};

int main(int argc, char **argv)
{
    Application app(800, 600);
    app.set_app_id("horizon.toolbar_demo");
    app.set_name("Toolbar Demo");
    app.set_icon_name("folder");
    app.set_show_in_dock(true);

    auto window = std::make_unique<ApplicationWindow>("Horizon Toolbar Demo");
    window->set_size(800, 600);

    auto tool = std::make_unique<ArkToolbar>();

    window->toolbar()->add_toolbar_widget(std::move(tool));

    // --- New Tab for VPanel demo ---
    auto vpanel_container = std::make_unique<Widget>();

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
    col_file_size.cell_factory = [](const FileEntry &f) { return std::make_unique<Label>(f.size); };
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

    window->add_child(std::move(vpanel_container));

    app.set_root(std::move(window));

    auto app_menu = new horizon::Menu();
    app_menu->set_title("Ark");
    app_menu->set_bold(true);
    app_menu->add_item("Preferences", "Ctrl+,");
    app_menu->add_separator();
    app_menu->add_item("Quit", "Ctrl+Q");

    auto file_menu = new horizon::Menu();
    file_menu->set_title("File");
    file_menu->add_item("New...", "Ctrl+N");
    file_menu->add_separator();
    file_menu->add_item("Exit", "Ctrl+Q");

    auto edit_menu = new horizon::Menu();
    edit_menu->set_title("Edit");
    edit_menu->add_item("Copy", "Ctrl+C");
    edit_menu->add_item("Paste", "Ctrl+V");

    std::vector<Menu *> menus = {app_menu, file_menu, edit_menu};

    app.set_global_menu(menus);

    app.run();
    return 0;
}
