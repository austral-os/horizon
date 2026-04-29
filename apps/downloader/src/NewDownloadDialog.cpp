#include "NewDownloadDialog.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Button.hpp"
#include "horizon/Label.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/TextBox.hpp"
#include "horizon/Window.hpp"

namespace horizon
{
    namespace downloader
    {

        NewDownloadDialog::NewDownloadDialog(std::function<void(std::string)> on_accept)
            : WaylandWindow("org.austral.downloader.new", 450, 200, true, false),
              m_on_accept(on_accept)
        {

            set_name("Nueva descarga");

            auto win = std::make_unique<horizon::Window>("Nueva descarga");

            auto container = std::make_unique<horizon::Widget>();
            container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
            container->set_margin(20);
            container->set_spacing(15);

            auto prompt = std::make_unique<horizon::Label>("Introduce la URL del archivo:");
            prompt->set_alignment(horizon::TextAlignment::Left);
            container->add_child(std::move(prompt));

            auto textbox = std::make_unique<horizon::TextBox<>>();
            textbox->set_text(
                "https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb");
            textbox->set_placeholder("https://ejemplo.com/archivo.zip");
            textbox->set_height(35);
            auto *textbox_ptr = textbox.get();
            container->add_child(std::move(textbox));

            auto buttons = std::make_unique<horizon::Widget>();
            buttons->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
            buttons->set_spacing(10);
            buttons->set_fixed_size(35);

            auto cancel_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
            cancel_btn->set_text("Cancelar");
            cancel_btn->set_size(100, 35);
            cancel_btn->when_click.connect([this](horizon::MouseButtonEventContext &)
                                           { this->quit(); });

            auto download_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
            download_btn->set_text("Descargar");
            download_btn->set_accent_color(WidgetAccentColor::Primary);
            download_btn->set_size(120, 35);
            download_btn->when_click.connect(
                [this, textbox_ptr](horizon::MouseButtonEventContext &)
                {
                    if (!textbox_ptr->text().empty())
                    {
                        m_on_accept(textbox_ptr->text());
                    }
                    this->quit();
                });

            buttons->add_child(horizon::Spacer());
            buttons->add_child(std::move(cancel_btn));
            buttons->add_child(std::move(download_btn));

            container->add_child(std::move(buttons));
            win->add_child(std::move(container));

            set_root(std::move(win));
        }

    } // namespace downloader
} // namespace horizon
