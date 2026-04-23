#include <horizon/Button.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Overview.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/SystemInfo.hpp>

namespace horizon
{
    const int LABEL_HEIGHT = 40;

    Overview::Overview()
    {
        // Load translations
        i18n().load_app_locales("horizon-overview");

        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("emblem-austral");
        icon->set_icon_size(256);

        auto icon_container = Spacer();
        icon_container->add_child(Spacer());
        icon_container->add_child(std::move(icon));
        icon_container->add_child(Spacer());
        icon_container->set_fixed_size(400);

        add_child(std::move(icon_container));

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto title = std::make_unique<Label>(SystemInfo::get_os());
        title->set_font_size(28);
        title->set_fixed_size(LABEL_HEIGHT);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        auto version = std::make_unique<Label>("Kernel " + SystemInfo::get_kernel());
        version->set_fixed_size(LABEL_HEIGHT);

        auto model = std::make_unique<Label>(SystemInfo::get_model());
        model->set_font_size(18);
        model->set_fixed_size(LABEL_HEIGHT);
        model->set_font_weight(FONT_WEIGHT_BOLD);

        auto cpu = std::make_unique<Label>(SystemInfo::get_cpu());
        cpu->set_font_size(18);
        cpu->set_fixed_size(LABEL_HEIGHT);
        cpu->set_font_weight(FONT_WEIGHT_BOLD);

        auto ram = std::make_unique<Label>(SystemInfo::get_ram());
        ram->set_font_size(18);
        ram->set_fixed_size(LABEL_HEIGHT);
        ram->set_font_weight(FONT_WEIGHT_BOLD);

        auto graphics = std::make_unique<Label>(SystemInfo::get_graphics());
        graphics->set_font_size(18);
        graphics->set_fixed_size(LABEL_HEIGHT);
        graphics->set_font_weight(FONT_WEIGHT_BOLD);

        auto button_container = Spacer();
        button_container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(LABEL_HEIGHT);

        auto btn_system = std::make_unique<Button<SolidObject>>();
        btn_system->set_text(i18n().tr("overview.system_report"));
        btn_system->set_fixed_size(200);

        auto btn_update = std::make_unique<Button<SolidObject>>();
        btn_update->set_text(i18n().tr("overview.update"));
        btn_update->set_fixed_size(200);

        button_container->add_child(std::move(btn_system));
        button_container->add_child(Spacer(40));
        button_container->add_child(std::move(btn_update));
        button_container->add_child(Spacer());

        content->add_child(Spacer());
        content->add_child(std::move(title));
        content->add_child(std::move(version));
        content->add_child(Spacer(30));
        content->add_child(std::move(model));
        content->add_child(std::move(cpu));
        content->add_child(std::move(ram));
        content->add_child(std::move(graphics));
        content->add_child(Spacer(30));
        content->add_child(std::move(button_container));
        content->add_child(Spacer());

        add_child(std::move(content));
    }
} // namespace horizon
