#include "ImageViewerToolbar.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Notification.hpp>
#include <horizon/Spacer.hpp>

namespace horizon
{
    namespace image
    {

        ImageViewerToolbar::ImageViewerToolbar()
        {
            set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            set_position_type(WidgetPositionTypes::FILL);
            set_margin(4);
            set_spacing(8);

            setup_ui();
        }

        void ImageViewerToolbar::setup_ui()
        {
            auto create_icon = [](const std::string &name, const std::string &tr_key)
            {
                auto icon = std::make_unique<horizon::Icon>();
                icon->set_icon_name(name);
                icon->set_icon_size(18);
                icon->set_use_theme_colors(true);
                if (!tr_key.empty())
                {
                    auto tooltip = std::make_unique<horizon::Notification>();
                    tooltip->set_message(i18n().tr(tr_key));
                    icon->set_tooltip(std::move(tooltip));
                }
                return icon;
            };

            // 1. Abrir (Izquierda)
            auto open_group = std::make_unique<horizon::GroupButton>();
            open_group->set_fixed_size(44);
            open_group->add_item(create_icon("document-open", "toolbar.open"));
            open_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &)
                {
                    horizon::EventContext ctx;
                    this->when_open_clicked.run(ctx);
                });
            add_child(std::move(open_group));

            // 2. Navegación
            auto nav_group = std::make_unique<horizon::GroupButton>();
            nav_group->set_fixed_size(84); // 2 botones
            nav_group->add_item(create_icon("go-previous", "toolbar.previous"));
            nav_group->add_item(create_icon("go-next", "toolbar.next"));
            nav_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                   { this->when_navigation_clicked.run(ev); });
            add_child(std::move(nav_group));

            add_child(horizon::Spacer());

            // 3. Zoom (Centrado)
            auto zoom_group = std::make_unique<horizon::GroupButton>();
            zoom_group->set_fixed_size(164); // 4 botones
            zoom_group->add_item(create_icon("zoom-out", "toolbar.zoom_out"));
            zoom_group->add_item(create_icon("zoom-in", "toolbar.zoom_in"));
            zoom_group->add_item(create_icon("zoom-fit-best", "toolbar.zoom_fit"));
            zoom_group->add_item(create_icon("zoom-original", "toolbar.zoom_original"));
            zoom_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                    { this->when_zoom_clicked.run(ev); });
            add_child(std::move(zoom_group));

            // 4. Transformación (Centrado)
            auto trans_group = std::make_unique<horizon::GroupButton>();
            trans_group->set_fixed_size(84); // 2 botones
            trans_group->add_item(create_icon("object-rotate-left", "toolbar.rotate_left"));
            trans_group->add_item(create_icon("object-rotate-right", "toolbar.rotate_right"));
            trans_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                     { this->when_transform_clicked.run(ev); });
            add_child(std::move(trans_group));

            add_child(horizon::Spacer());

            // 5. Fullscreen / Settings (Derecha)
            auto extra_group = std::make_unique<horizon::GroupButton>();
            extra_group->set_fixed_size(84); // 2 botones
            extra_group->add_item(create_icon("view-fullscreen", "toolbar.fullscreen"));
            extra_group->add_item(create_icon("emblem-system", "toolbar.settings"));
            extra_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent &ev)
                                                     { this->when_extra_clicked.run(ev); });
            add_child(std::move(extra_group));
        }

    } // namespace image
} // namespace horizon
