#include "DocumentToolbar.hpp"
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    namespace pdf
    {

        DocumentToolbar::DocumentToolbar() : Widget()
        {
            set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            set_position_type(WidgetPositionTypes::FILL);
            set_margin(4);
            set_spacing(8);

            setup_ui();
        }

        void DocumentToolbar::setup_ui()
        {
            // 1. Grupo de archivo (Abrir)
            auto file_group = std::make_unique<horizon::GroupButton>();
            file_group->set_fixed_size(44);

            auto open_icon = std::make_unique<horizon::Icon>();
            open_icon->set_icon_name("document-open");
            open_icon->set_icon_size(18);
            open_icon->set_use_theme_colors(true);
            file_group->add_item(std::move(open_icon));

            file_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &ev)
                {
                    horizon::EventContext ctx;
                    this->when_open_clicked.run(ctx);
                });
            add_child(std::move(file_group));

            // 1.5 Botón de Barra Lateral (Ocultar/Mostrar)
            auto sidebar_group = std::make_unique<horizon::GroupButton>();
            sidebar_group->set_fixed_size(44);

            auto sidebar_icon = std::make_unique<horizon::Icon>();
            sidebar_icon->set_icon_name("view-sidebar");
            sidebar_icon->set_icon_size(18);
            sidebar_icon->set_use_theme_colors(true);
            sidebar_group->add_item(std::move(sidebar_icon));

            sidebar_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &ev)
                {
                    horizon::EventContext ctx;
                    this->when_sidebar_toggled.run(ctx);
                });
            add_child(std::move(sidebar_group));

            add_child(horizon::Spacer());

            // 2. Grupo de Zoom
            auto zoom_group = std::make_unique<horizon::GroupButton>();
            zoom_group->set_fixed_size(120); // 3 botones

            auto zoom_out_icon = std::make_unique<horizon::Icon>();
            zoom_out_icon->set_icon_name("zoom-out");
            zoom_out_icon->set_icon_size(18);
            zoom_out_icon->set_use_theme_colors(true);
            zoom_group->add_item(std::move(zoom_out_icon));

            auto zoom_in_icon = std::make_unique<horizon::Icon>();
            zoom_in_icon->set_icon_name("zoom-in");
            zoom_in_icon->set_icon_size(18);
            zoom_in_icon->set_use_theme_colors(true);
            zoom_group->add_item(std::move(zoom_in_icon));

            auto zoom_fit_icon = std::make_unique<horizon::Icon>();
            zoom_fit_icon->set_icon_name("zoom-fit-best");
            zoom_fit_icon->set_icon_size(18);
            zoom_fit_icon->set_use_theme_colors(true);
            zoom_group->add_item(std::move(zoom_fit_icon));

            zoom_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                    { this->when_zoom_clicked.run(ev); });
            add_child(std::move(zoom_group));

            add_child(horizon::Spacer());

            // 3. Grupo de Vista/Ajustes
            auto view_group = std::make_unique<horizon::GroupButton>();
            view_group->set_fixed_size(84); // 2 botones

            auto fs_icon = std::make_unique<horizon::Icon>();
            fs_icon->set_icon_name("view-fullscreen");
            fs_icon->set_icon_size(18);
            fs_icon->set_use_theme_colors(true);
            view_group->add_item(std::move(fs_icon));

            auto settings_icon = std::make_unique<horizon::Icon>();
            settings_icon->set_icon_name("emblem-system");
            settings_icon->set_icon_size(18);
            settings_icon->set_use_theme_colors(true);
            view_group->add_item(std::move(settings_icon));

            view_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                    { this->when_view_clicked.run(ev); });
            add_child(std::move(view_group));
        }

    } // namespace pdf
} // namespace horizon
