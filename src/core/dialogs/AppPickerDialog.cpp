#include <algorithm>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/AppPickerDialog.hpp>

namespace horizon
{
    AppPickerDialog::AppPickerDialog() : WaylandWindow("horizon.app_picker", 450, 550, false, true)
    {
        set_name("Seleccionar Aplicación");
        setup_ui();
        load_apps();
    }

    void AppPickerDialog::setup_ui()
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_wnd->set_margin(0);
        root_wnd->set_spacing(0);

        auto container = std::make_unique<Widget>();
        container->set_margin(15);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(10);

        // --- Search Area ---
        auto search_box = std::make_unique<SearchBox>();
        search_box->set_placeholder("Buscar aplicaciones...");
        search_box->set_fixed_size(35);
        m_search_box = search_box.get();
        m_search_box->when_text_changed.connect([this](KeyEventContext &)
                                                { filter_apps(m_search_box->text()); });
        container->add_child(std::move(search_box));

        // --- Table Area ---
        auto table = std::make_unique<TableView<DesktopEntry>>();
        m_app_table = table.get();
        m_app_table->set_header_visible(false);
        m_app_table->set_position_type(WidgetPositionTypes::FILL);

        TableColumn<DesktopEntry> col;
        col.width = 400;
        col.cell_factory = [](const DesktopEntry &entry)
        {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            cell->set_spacing(10);

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(entry.icon.empty() ? "application-x-executable" : entry.icon);
            icon->set_icon_size(24);
            icon->set_fixed_size(24);

            cell->add_child(std::move(icon));

            auto label = std::make_unique<Label>(entry.name);
            label->set_font_size(14);
            cell->add_child(std::move(label));

            return cell;
        };
        m_app_table->add_column(col);
        container->add_child(std::move(table));

        // --- Footer Area ---
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);
        buttons->set_spacing(10);

        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancelar");
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text("Aceptar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        btn_accept->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                int idx = m_app_table->selected_index();
                if (idx != -1)
                {
                    when_accepted.run(m_filtered_apps[idx]);
                    this->quit();
                }
            });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));

        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void AppPickerDialog::load_apps()
    {
        m_all_apps = DesktopManager::load_all_desktop_entries();

        // Sort by name
        std::sort(m_all_apps.begin(), m_all_apps.end(),
                  [](const DesktopEntry &a, const DesktopEntry &b) { return a.name < b.name; });

        m_filtered_apps = m_all_apps;
        m_app_table->set_data(m_filtered_apps);
    }

    void AppPickerDialog::filter_apps(const std::string &query)
    {
        if (query.empty())
        {
            m_filtered_apps = m_all_apps;
        }
        else
        {
            m_filtered_apps.clear();
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

            for (const auto &app : m_all_apps)
            {
                std::string lower_name = app.name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                if (lower_name.find(lower_query) != std::string::npos)
                {
                    m_filtered_apps.push_back(app);
                }
            }
        }
        m_app_table->set_data(m_filtered_apps);
    }
} // namespace horizon
