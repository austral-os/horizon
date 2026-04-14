#include <horizon/dialogs/ColorPickerDialog.hpp>
#include <horizon/ColorPicker.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{
    struct ColorPickerDialog::Impl
    {
        ColorPickerDialog *parent;
        ColorPicker *picker{nullptr};
        Color initial_color;

        Impl(ColorPickerDialog *p) : parent(p) {}

        void setup_ui(const std::string &title)
        {
            auto window_widget = std::make_unique<Window>(title);
            auto *app_window = window_widget.get();
            app_window->set_size(620, 520);

            auto root = std::make_unique<Widget>();
            root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            root->set_margin(12);
            root->set_spacing(12);

            // Color Picker
            auto cp = std::make_unique<ColorPicker>();
            picker = cp.get();
            picker->set_position_type(FILL);
            root->add_child(std::move(cp));

            // Buttons
            auto button_area = std::make_unique<Widget>();
            button_area->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            button_area->set_fixed_size(40);
            button_area->set_spacing(12);

            auto spacer = std::make_unique<Widget>();
            button_area->add_child(std::move(spacer));

            auto ok_btn = std::make_unique<Button<AquaObject>>();
            ok_btn->set_text(i18n().tr("core.dialog.accept"));
            ok_btn->set_width(110);
            ok_btn->set_accent_color(WidgetAccentColor::Primary);
            ok_btn->when_mouse_press.connect([this](MouseButtonEventContext &) {
                ColorPickerDialogAcceptedContext ctx;
                ctx.color = picker->color();
                parent->when_accepted.run(ctx);
                parent->quit();
            });
            button_area->add_child(std::move(ok_btn));

            auto cancel_btn = std::make_unique<Button<AquaObject>>();
            cancel_btn->set_text(i18n().tr("core.dialog.cancel"));
            cancel_btn->set_width(110);
            cancel_btn->when_mouse_press.connect([this](MouseButtonEventContext &) {
                ColorPickerDialogCancelledContext ctx;
                parent->when_cancelled.run(ctx);
                parent->quit();
            });
            button_area->add_child(std::move(cancel_btn));

            root->add_child(std::move(button_area));

            app_window->add_child(std::move(root));
            parent->set_root(std::move(window_widget));
        }
    };

    ColorPickerDialog::ColorPickerDialog(const std::string &title) : m_impl(std::make_unique<Impl>(this))
    {
        m_impl->setup_ui(title);
    }

    ColorPickerDialog::~ColorPickerDialog() = default;

    Color ColorPickerDialog::color() const
    {
        return m_impl->picker->color();
    }

    void ColorPickerDialog::set_color(const Color &color)
    {
        m_impl->picker->set_color(color);
    }
}
