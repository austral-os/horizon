#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/MessageDialog.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Window.hpp>
#include <memory>

namespace horizon
{
    MessageDialog::MessageDialog(const std::string &title, const std::string &message,
                                 MessageType type, bool show_cancel)
        : WaylandWindow("horizon.dialog", 700, 215, true, true)
    {
        set_name(title);
        setup_ui(message, type, show_cancel);
        set_min_size(400, 215);
    }

    void MessageDialog::setup_ui(const std::string &message, MessageType type, bool show_cancel)
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        // Content area: Icon + Message
        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        content->set_margin(15);
        content->set_spacing(15);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(get_icon_for_type(type));
        icon->set_icon_size(64);
        icon->set_fixed_size(64);
        icon->set_vertical_alignment(VerticalAlignment::Top);

        auto label = std::make_unique<Label>(message);
        m_label = label.get();
        label->set_position_type(FILL);
        label->set_vertical_alignment(VerticalAlignment::Top);

        content->add_child(std::move(icon));
        content->add_child(std::move(label));

        // Buttons area
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);

        buttons->add_child(Spacer());

        if (show_cancel)
        {
            auto btn_cancel = std::make_unique<Button<AquaObject>>();
            m_cancel_btn = btn_cancel.get();
            btn_cancel->set_text("Cancelar");
            btn_cancel->set_fixed_size(100);
            btn_cancel->when_click.connect(
                [this](MouseButtonEventContext &)
                {
                    MessageResponseEvent res;
                    res.response = MessageResponse::Cancel;
                    when_responded.run(res);
                    this->post_task([this]() { this->quit(); });
                });
            buttons->add_child(std::move(btn_cancel));
            buttons->add_child(Spacer(15));
        }

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        m_accept_btn = btn_accept.get();
        btn_accept->set_text("Aceptar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(get_color_for_type(type));
        btn_accept->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                MessageResponseEvent res;
                res.response = MessageResponse::Accept;
                when_responded.run(res);
                this->post_task([this]() { this->quit(); });
            });

        buttons->add_child(std::move(btn_accept));
        buttons->add_child(Spacer(15));

        root_wnd->add_child(std::move(content));
        root_wnd->add_child(std::move(buttons));
        root_wnd->add_child(Spacer(15));

        set_root(std::move(root_wnd));
    }

    void MessageDialog::set_message(const std::string &message)
    {
        if (m_label)
        {
            m_label->set_text(message);
        }
    }

    void MessageDialog::set_accept_text(const std::string &text)
    {
        if (m_accept_btn)
        {
            m_accept_btn->set_text(text);
        }
    }

    void MessageDialog::set_cancel_text(const std::string &text)
    {
        if (m_cancel_btn)
        {
            m_cancel_btn->set_text(text);
        }
    }

    std::string MessageDialog::get_icon_for_type(MessageType type)
    {
        switch (type)
        {
        case MessageType::Info:
            return "dialog-information";
        case MessageType::Warning:
            return "dialog-warning";
        case MessageType::Error:
            return "dialog-error";
        case MessageType::Question:
            return "dialog-question";
        default:
            return "dialog-information";
        }
    }

    WidgetAccentColor MessageDialog::get_color_for_type(MessageType type)
    {
        switch (type)
        {
        case MessageType::Info:
            return WidgetAccentColor::Info;
        case MessageType::Warning:
            return WidgetAccentColor::Warning;
        case MessageType::Error:
            return WidgetAccentColor::Error;
        case MessageType::Question:
            return WidgetAccentColor::Primary;
        default:
            return WidgetAccentColor::Primary;
        }
    }
} // namespace horizon
