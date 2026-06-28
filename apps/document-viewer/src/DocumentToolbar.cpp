#include "DocumentToolbar.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Notification.hpp>
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
            auto create_icon = [](const std::string &name, const std::string &tr_key)
            {
                auto icon = std::make_unique<horizon::Icon>();
                icon->set_icon_name(name);
                icon->set_icon_size(18);
                icon->set_use_theme_colors(true);

                auto tooltip = std::make_unique<horizon::Notification>();
                tooltip->set_message(i18n().tr(tr_key));
                icon->set_tooltip(std::move(tooltip));

                return icon;
            };

            // 1. Grupo de archivo (Abrir)
            auto file_group = std::make_unique<horizon::GroupButton>();
            file_group->set_fixed_size(44);

            file_group->add_item(create_icon("document-open", "toolbar.open"));

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

            sidebar_group->add_item(create_icon("view-sidebar", "toolbar.sidebar"));

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

            zoom_group->add_item(create_icon("zoom-out", "toolbar.zoom_out"));
            zoom_group->add_item(create_icon("zoom-in", "toolbar.zoom_in"));
            zoom_group->add_item(create_icon("zoom-fit-best", "toolbar.zoom_fit"));

            zoom_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                    { this->when_zoom_clicked.run(ev); });
            add_child(std::move(zoom_group));

            add_child(horizon::Spacer());

            // 3. Grupo de Vista/Ajustes
            auto view_group = std::make_unique<horizon::GroupButton>();
            view_group->set_fixed_size(84); // 2 botones

            view_group->add_item(create_icon("view-fullscreen", "toolbar.fullscreen"));
            view_group->add_item(create_icon("configure", "toolbar.settings"));

            view_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                    { this->when_view_clicked.run(ev); });
            add_child(std::move(view_group));
        }

    } // namespace pdf
} // namespace horizon
