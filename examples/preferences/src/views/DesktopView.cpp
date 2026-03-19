#include "horizon/Spacer.hpp"
#include "horizon/Widget.hpp"
#include <ConfigManager.hpp>
#include <filesystem>
#include <horizon/TreeViewItem.hpp>
#include <views/DesktopView.hpp>

namespace horizon::preferences
{
    DesktopView::DesktopView() : horizon::Widget()
    {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::WidgetPositionTypes::FILL);
        set_margin(10);
        set_spacing(10);

        // --- Section B (Top Preview) ---
        auto section_b_widget = std::make_unique<horizon::Widget>();
        create_section_b(section_b_widget.get());
        add_child(std::move(section_b_widget));

        // --- Sections A & C (Middle: TreeView & ImagesView) ---
        auto section_ac_widget = std::make_unique<horizon::Widget>();
        create_section_ac(section_ac_widget.get());
        add_child(std::move(section_ac_widget));

        // --- Section D (Bottom: Actions & Settings) ---
        auto section_d_widget = std::make_unique<horizon::Widget>();
        create_section_d(section_d_widget.get());
        add_child(std::move(section_d_widget));

        // Load configuration
        from_json(ConfigManager::instance().get_section("desktop"));
    }

    void DesktopView::create_section_b(horizon::Widget *parent)
    {
        parent->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        parent->set_fixed_size(100);
        parent->set_spacing(20);

        // Left side: Image preview
        auto img = std::make_unique<horizon::Image>();
        img->set_size(120, 80);
        img->set_mode(horizon::ImageMode::Fit);
        m_preview_image = img.get();
        parent->add_child(std::move(img));

        // Right side: Name and Fit combo
        auto right_vbox = std::make_unique<horizon::Widget>();
        right_vbox->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        right_vbox->set_spacing(10);

        auto name_label = std::make_unique<horizon::Label>("Seleccione una imagen");
        m_image_name_label = name_label.get();
        right_vbox->add_child(std::move(name_label));

        auto fit_combo = std::make_unique<horizon::Combo>();
        fit_combo->add_item("fill", "Rellenar pantalla");
        fit_combo->add_item("fit", "Ajustar a pantalla");
        fit_combo->add_item("stretch", "Estirar para rellenar");
        fit_combo->add_item("center", "Centrar");
        m_fit_combo = fit_combo.get();
        m_fit_combo->when_item_selected.connect([this](const horizon::ComboItemSelectedContext &)
                                                { save_config(); });
        right_vbox->add_child(std::move(fit_combo));

        parent->add_child(std::move(right_vbox));
    }

    void DesktopView::create_section_ac(horizon::Widget *parent)
    {
        parent->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        parent->set_position_type(horizon::WidgetPositionTypes::FILL);
        parent->set_spacing(10);

        // Section A: TreeView
        auto tree = std::make_unique<horizon::TreeView>();
        tree->set_fixed_size(200);
        m_tree_view = tree.get();

        parent->add_child(std::move(tree));

        // Section C: ImagesView
        auto images = std::make_unique<ImagesView>();
        m_images_view = images.get();

        m_images_view->when_image_selected.connect(
            [this](const std::string &path)
            {
                m_preview_image->set_path(path);
                std::filesystem::path p(path);
                m_image_name_label->set_text(p.filename().string());
                m_current_image_name = p.filename().string();
                m_current_image_full_path = path;
                save_config();
            });

        parent->add_child(std::move(images));
    }

    void DesktopView::create_section_d(horizon::Widget *parent)
    {
        parent->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        parent->set_fixed_size(100);
        parent->set_spacing(20);

        // Left side: Add/Remove buttons
        auto buttons_vbox = std::make_unique<horizon::Widget>();
        buttons_vbox->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        buttons_vbox->set_fixed_size(190);

        auto buttons_hbox = std::make_unique<horizon::Widget>();
        buttons_hbox->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        buttons_hbox->set_spacing(5);
        buttons_hbox->set_fixed_size(38);

        auto add_btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
        add_btn->set_text("+");
        m_add_button = add_btn.get();
        buttons_hbox->add_child(std::move(add_btn));

        auto rem_btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
        rem_btn->set_text("-");
        m_remove_button = rem_btn.get();
        buttons_hbox->add_child(std::move(rem_btn));
        buttons_hbox->add_child(horizon::Spacer());

        buttons_vbox->add_child(std::move(buttons_hbox));
        buttons_vbox->add_child(horizon::Spacer());

        parent->add_child(std::move(buttons_vbox));

        // Right side: Settings Checkboxes & Timer
        auto settings_vbox = std::make_unique<horizon::Widget>();
        settings_vbox->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        settings_vbox->set_spacing(5);

        auto row1 = std::make_unique<horizon::Widget>();
        row1->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        row1->set_spacing(15);

        auto change_chk = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        change_chk->set_text("Cambiar imagen:");
        change_chk->set_fixed_size(180); // Width because it's in a horizontal row
        m_change_check = change_chk.get();
        row1->add_child(std::move(change_chk));

        auto timer_combo = std::make_unique<horizon::Combo>();
        timer_combo->add_item("30m", "Cada 30 minutos");
        timer_combo->add_item("1h", "Cada hora");
        timer_combo->set_fixed_size(160); // Width because it's in a horizontal row
        m_timer_combo = timer_combo.get();
        row1->add_child(std::move(timer_combo));

        settings_vbox->add_child(std::move(row1));

        auto random_chk = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        random_chk->set_text("Orden aleatorio");
        random_chk->set_fixed_size(30); // Height because it's in a vertical vbox
        m_random_check = random_chk.get();
        settings_vbox->add_child(std::move(random_chk));

        auto translucent_chk = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        translucent_chk->set_text("Barra de menú translúcida");
        translucent_chk->set_fixed_size(30); // Height because it's in a vertical vbox
        m_translucent_check = translucent_chk.get();
        settings_vbox->add_child(std::move(translucent_chk));

        parent->add_child(std::move(settings_vbox));
    }

    void DesktopView::update_layout() {}

    void DesktopView::from_json(const nlohmann::json &j)
    {
        if (j.is_null() || !j.contains("backgrounds"))
            return;

        const auto &backgrounds = j["backgrounds"];

        // Load current
        if (backgrounds.contains("current"))
        {
            const auto &current = backgrounds["current"];
            m_current_image_name = current.value("name", "");
            m_current_image_full_path = current.value("path", "");
            m_current_source = current.value("source", "");
            std::string fit = current.value("fit", "fill");

            if (m_fit_combo)
                m_fit_combo->set_selected_item_by_id(fit);
            if (m_image_name_label)
                m_image_name_label->set_text(m_current_image_name);
            if (m_preview_image && !m_current_image_full_path.empty())
            {
                m_preview_image->set_path(m_current_image_full_path);
            }
        }

        if (backgrounds.contains("sources") && backgrounds["sources"].is_array())
        {
            m_sources.clear();
            for (const auto &src_json : backgrounds["sources"])
            {
                BackgroundSource source;
                source.name = src_json.value("name", "");

                if (src_json.contains("routes") && src_json["routes"].is_array())
                {
                    for (const auto &route_json : src_json["routes"])
                    {
                        BackgroundRoute route;
                        route.name = route_json.value("name", "");
                        route.path = route_json.value("path", "");
                        source.routes.push_back(route);
                    }
                }
                m_sources.push_back(source);
            }
        }

        update_tree_view();
    }

    void DesktopView::update_tree_view()
    {
        if (!m_tree_view)
            return;

        m_tree_view->clear_root_items();
        m_tree_view->when_item_selected.disconnect_all();

        m_first_route_to_select = nullptr;
        m_initial_selection_done = false;

        for (const auto &source : m_sources)
        {
            auto source_item = std::make_unique<horizon::TreeViewItem>("folder", source.name);
            source_item->set_expanded(true);

            for (const auto &route : source.routes)
            {
                auto route_item =
                    std::make_unique<horizon::TreeViewItem>("folder-open", route.name);
                auto *route_item_ptr = route_item.get();

                std::string path = route.path;
                std::string source_name = route.name;
                m_tree_view->when_item_selected.connect(
                    [this, route_item_ptr, path, source_name](horizon::TreeViewItem *selected)
                    {
                        if (selected == route_item_ptr)
                        {
                            m_current_source = source_name;
                            m_images_view->set_path(path);
                        }
                    });

                if (!m_first_route_to_select)
                    m_first_route_to_select = route_item_ptr;
                source_item->add_item(std::move(route_item));
            }
            m_tree_view->add_root_item(std::move(source_item));
        }
    }

    void DesktopView::calculate_layout()
    {
        horizon::Widget::calculate_layout();

        // Trigger initial selection once attached to window
        if (!m_initial_selection_done && m_first_route_to_select && application())
        {
            m_initial_selection_done = true;
            m_tree_view->set_selected_item(m_first_route_to_select);
        }
    }

    nlohmann::json DesktopView::to_json() const
    {
        nlohmann::json sources_json = nlohmann::json::array();
        for (const auto &source : m_sources)
        {
            nlohmann::json routes_json = nlohmann::json::array();
            for (const auto &route : source.routes)
            {
                routes_json.push_back({{"name", route.name}, {"path", route.path}});
            }
            sources_json.push_back({{"name", source.name}, {"routes", routes_json}});
        }

        std::string fit = "fill";
        if (m_fit_combo && m_fit_combo->selected_item())
        {
            fit = m_fit_combo->selected_item()->id;
        }

        return {{"backgrounds",
                 {{"current",
                   {{"change-time", 0},
                    {"fit", fit},
                    {"name", m_current_image_name},
                    {"path", m_current_image_full_path},
                    {"source", m_current_source},
                    {"type", "image"}}},
                  {"sources", sources_json}}}};
    }

    void DesktopView::save_config()
    {
        ConfigManager::instance().set_section("desktop", to_json());
        ConfigManager::instance().save();
    }
} // namespace horizon::preferences
