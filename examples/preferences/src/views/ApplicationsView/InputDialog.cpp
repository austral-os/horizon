#include <views/ApplicationsView/InputDialog.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>

namespace horizon::preferences
{
    InputDialog::InputDialog(const std::string &title, const std::string &prompt)
        : WaylandWindow("horizon.input_dialog", 400, 180, true, true)
    {
        set_name(title);
        setup_ui(prompt);
        set_min_size(300, 180);
    }

    void InputDialog::setup_ui(const std::string &prompt)
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_wnd->set_margin(20);
        root_wnd->set_spacing(15);

        auto label = std::make_unique<Label>(prompt);
        label->set_fixed_size(25);
        root_wnd->add_child(std::move(label));

        auto input = std::make_unique<TextBox<TextPolicy>>();
        m_input = input.get();
        m_input->set_fixed_size(35);
        m_input->set_focusable(true);
        root_wnd->add_child(std::move(input));

        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);
        buttons->set_spacing(10);

        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancelar");
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                EventContext ctx;
                when_cancelled.run(ctx);
                this->quit();
            });
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text("Aceptar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        btn_accept->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                std::string val = m_input->text();
                when_accepted.run(val);
                this->quit();
            });
        buttons->add_child(std::move(btn_accept));

        root_wnd->add_child(std::move(buttons));
        set_root(std::move(root_wnd));
    }
} // namespace horizon::preferences
