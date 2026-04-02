#include <algorithm>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Window.hpp>
#include <views/ApplicationsView/LayoutPickerDialog.hpp>

namespace horizon::preferences
{
    LayoutPickerDialog::LayoutPickerDialog() : WaylandWindow("horizon.layout_picker", 450, 550, true, true)
    {
        set_name("Seleccionar Layout de Teclado");
        setup_ui();
        load_layouts();
    }

    void LayoutPickerDialog::setup_ui()
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
        search_box->set_placeholder("Buscar layouts...");
        search_box->set_fixed_size(35);
        m_search_box = search_box.get();
        m_search_box->when_text_changed.connect([this](KeyEventContext &)
                                                { filter_layouts(m_search_box->text()); });
        container->add_child(std::move(search_box));

        // --- Table Area ---
        auto table = std::make_unique<TableView<KeyboardLayout>>();
        m_layout_table = table.get();
        m_layout_table->set_header_visible(false);
        m_layout_table->set_position_type(WidgetPositionTypes::FILL);

        TableColumn<KeyboardLayout> col;
        col.width = 400;
        col.cell_factory = [](const KeyboardLayout &layout)
        {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            cell->set_spacing(10);

            auto label = std::make_unique<Label>(layout.description + " (" + layout.id + ")");
            label->set_font_size(14);
            cell->add_child(std::move(label));

            return cell;
        };
        m_layout_table->add_column(col);
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
                int idx = m_layout_table->selected_index();
                if (idx != -1)
                {
                    when_accepted.run(m_filtered_layouts[idx]);
                    this->quit();
                }
            });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));

        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void LayoutPickerDialog::load_layouts()
    {
        m_all_layouts = XkbParser::get_layouts();

        // Sort by name
        std::sort(m_all_layouts.begin(), m_all_layouts.end(),
                  [](const KeyboardLayout &a, const KeyboardLayout &b) { return a.description < b.description; });

        m_filtered_layouts = m_all_layouts;
        m_layout_table->set_data(m_filtered_layouts);
    }

    void LayoutPickerDialog::filter_layouts(const std::string &query)
    {
        if (query.empty())
        {
            m_filtered_layouts = m_all_layouts;
        }
        else
        {
            m_filtered_layouts.clear();
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

            for (const auto &layout : m_all_layouts)
            {
                std::string lower_desc = layout.description;
                std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
                std::string lower_id = layout.id;
                std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);

                if (lower_desc.find(lower_query) != std::string::npos || lower_id.find(lower_query) != std::string::npos)
                {
                    m_filtered_layouts.push_back(layout);
                }
            }
        }
        m_layout_table->set_data(m_filtered_layouts);
    }
} // namespace horizon::preferences
