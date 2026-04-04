#include <ConfigManager.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Window.hpp>
#include <thread>
#include <views/ApplicationsView/LayoutPickerDialog.hpp>
#include <views/KeyboardView/KeyboardLanguageView.hpp>

namespace horizon::preferences
{
    KeyboardLanguageView::KeyboardLanguageView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(15);

        setup_ui();
        load_config();
    }

    void KeyboardLanguageView::setup_ui()
    {
        // --- Toolbar ---
        auto toolbar = std::make_unique<Widget>();
        toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        toolbar->set_fixed_size(35);
        toolbar->set_spacing(10);

        auto btn_add = std::make_unique<Button<AquaObject>>();
        btn_add->set_text("Agregar");
        btn_add->set_fixed_size(100);
        btn_add->when_click.connect([this](MouseButtonEventContext &) { add_layout(); });
        toolbar->add_child(std::move(btn_add));

        auto btn_remove = std::make_unique<Button<AquaObject>>();
        btn_remove->set_text("Quitar");
        btn_remove->set_fixed_size(100);
        btn_remove->when_click.connect([this](MouseButtonEventContext &) { remove_layout(); });
        toolbar->add_child(std::move(btn_remove));

        toolbar->add_child(Spacer());

        auto btn_default = std::make_unique<Button<AquaObject>>();
        btn_default->set_text("Predeterminado");
        btn_default->set_fixed_size(140);
        btn_default->when_click.connect([this](MouseButtonEventContext &)
                                        { set_default_layout(); });
        toolbar->add_child(std::move(btn_default));

        add_child(std::move(toolbar));

        // --- Table ---
        auto table = std::make_unique<TableView<KeyboardLayoutSelection>>();
        m_layout_table = table.get();
        m_layout_table->set_position_type(WidgetPositionTypes::FILL);

        // Column: Name
        TableColumn<KeyboardLayoutSelection> col_name;
        col_name.title = "Nombre";
        col_name.width = 250;
        col_name.cell_factory = [](const KeyboardLayoutSelection &sel)
        { return std::make_unique<Label>(sel.description); };
        m_layout_table->add_column(col_name);

        // Column: ID
        TableColumn<KeyboardLayoutSelection> col_id;
        col_id.title = "ID";
        col_id.width = 80;
        col_id.cell_factory = [](const KeyboardLayoutSelection &sel)
        { return std::make_unique<Label>(sel.id); };
        m_layout_table->add_column(col_id);

        // Column: Default
        TableColumn<KeyboardLayoutSelection> col_def;
        col_def.title = "Estado";
        col_def.width = 150;
        col_def.cell_factory = [](const KeyboardLayoutSelection &sel)
        {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            cell->set_spacing(5);

            if (sel.is_default)
            {
                auto lbl = std::make_unique<Label>("Predeterminado");
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
                lbl->set_font_size(12);
                cell->add_child(std::move(lbl));
            }
            return cell;
        };
        m_layout_table->add_column(col_def);

        m_layout_table->when_row_click.connect(
            [this](TableViewRowMouseClickContext<KeyboardLayoutSelection> &ctx)
            { set_default_layout(); });

        add_child(std::move(table));
    }

    void KeyboardLanguageView::load_config()
    {
        auto keyboard = ConfigManager::instance().get_section("keyboard");
        if (keyboard.contains("layouts") && keyboard["layouts"].is_array())
        {
            m_selected_layouts.clear();
            for (const auto &l : keyboard["layouts"])
            {
                KeyboardLayoutSelection sel;
                sel.id = l["id"].get<std::string>();
                sel.description = l["description"].get<std::string>();
                sel.is_default = l.value("default", false);
                m_selected_layouts.push_back(sel);
            }
        }

        if (!m_selected_layouts.empty())
        {
            bool has_default = false;
            for (const auto &s : m_selected_layouts)
                if (s.is_default)
                    has_default = true;
            if (!has_default)
                m_selected_layouts[0].is_default = true;
        }

        m_layout_table->set_data(m_selected_layouts);
    }

    void KeyboardLanguageView::save_config()
    {
        auto keyboard = ConfigManager::instance().get_section("keyboard");

        nlohmann::json layouts_json = nlohmann::json::array();
        for (const auto &sel : m_selected_layouts)
        {
            nlohmann::json l;
            l["id"] = sel.id;
            l["description"] = sel.description;
            l["default"] = sel.is_default;
            layouts_json.push_back(l);
        }

        keyboard["layouts"] = layouts_json;
        ConfigManager::instance().set_section("keyboard", keyboard);
    }

    void KeyboardLanguageView::add_layout()
    {
        auto picker = std::make_unique<LayoutPickerDialog>();
        auto *picker_ptr = picker.get();

        picker_ptr->when_accepted.connect(
            [this](KeyboardLayout layout)
            {
                // Check if already added
                bool exists = false;
                for (const auto &sel : m_selected_layouts)
                {
                    if (sel.id == layout.id)
                    {
                        exists = true;
                        break;
                    }
                }
                if (exists)
                    return;

                KeyboardLayoutSelection sel;
                sel.id = layout.id;
                sel.description = layout.description;
                sel.is_default = m_selected_layouts.empty();

                m_selected_layouts.push_back(sel);
                m_layout_table->set_data(m_selected_layouts);
                save_config();
            });

        std::thread(
            [d = std::move(picker)]() mutable
            {
                d->initialize();
                d->run();
            })
            .detach();
    }

    void KeyboardLanguageView::remove_layout()
    {
        int idx = m_layout_table->selected_index();
        if (idx != -1)
        {
            bool was_default = m_selected_layouts[idx].is_default;
            m_selected_layouts.erase(m_selected_layouts.begin() + idx);

            if (was_default && !m_selected_layouts.empty())
            {
                m_selected_layouts[0].is_default = true;
            }

            m_layout_table->set_data(m_selected_layouts);
            save_config();
        }
    }

    void KeyboardLanguageView::set_default_layout()
    {
        int idx = m_layout_table->selected_index();
        if (idx != -1)
        {
            for (auto &sel : m_selected_layouts)
                sel.is_default = false;
            m_selected_layouts[idx].is_default = true;

            m_layout_table->set_data(m_selected_layouts);
            save_config();
            apply_layout_to_labwc(m_selected_layouts[idx].id);
        }
    }
    void KeyboardLanguageView::apply_layout_to_labwc(const std::string &layout_id)
    {
        const char *home = std::getenv("HOME");
        if (!home)
            return;

        std::filesystem::path config_path(home);
        config_path /= ".config/labwc/environment";

        // Pre-ensure directory exists
        if (!std::filesystem::exists(config_path.parent_path()))
        {
            std::filesystem::create_directories(config_path.parent_path());
        }

        std::vector<std::string> lines;
        bool found = false;
        std::string target_prefix = "XKB_DEFAULT_LAYOUT=";

        if (std::filesystem::exists(config_path))
        {
            std::ifstream file(config_path);
            std::string line;
            while (std::getline(file, line))
            {
                if (line.compare(0, target_prefix.length(), target_prefix) == 0)
                {
                    lines.push_back(target_prefix + layout_id);
                    found = true;
                }
                else
                {
                    lines.push_back(line);
                }
            }
        }

        if (!found)
        {
            lines.push_back(target_prefix + layout_id);
        }

        std::ofstream out(config_path);
        for (const auto &l : lines)
        {
            out << l << "\n";
        }
        out.close();

        // Run labwc --reconfigure
        std::system("labwc --reconfigure");
    }
} // namespace horizon::preferences
