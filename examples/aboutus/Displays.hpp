#include "horizon/Button.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/Widget.hpp"
#include <horizon/SystemInfo.hpp>
#include <horizon/ApplicationLauncher.hpp>
namespace horizon
{

    class Displays : public Widget
    {
    public:
        Displays()
        {
            const int LABEL_HEIGHT = 40;
            set_layout_type(WIDGET_LAYOUT_VERTICAL);

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("monitor");
            icon->set_icon_size(256);

            auto icon_container = Spacer();
            icon_container->add_child(Spacer());
            icon_container->add_child(std::move(icon));
            icon_container->add_child(Spacer());
            icon_container->set_fixed_size(260);

            auto info_container = std::make_unique<Widget>();
            info_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

            auto btn_container = std::make_unique<Widget>();
            btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            btn_container->set_fixed_size(LABEL_HEIGHT * 2);
            btn_container->set_margin(20);

            auto btn_display_settings = std::make_unique<Button<SolidObject>>();
            btn_display_settings->set_text("Display Settings");
            btn_display_settings->set_fixed_size(200);

            btn_display_settings->when_click.connect(
                [](MouseButtonEventContext &)
                {
                    ApplicationLauncher::launch_from_desktop_file("preferences", {"--display"});
                });

            btn_container->add_child(Spacer());
            btn_container->add_child(std::move(btn_display_settings));

            auto content = std::make_unique<Widget>();
            content->set_layout_type(WIDGET_LAYOUT_VERTICAL);

            auto title = std::make_unique<Label>(SystemInfo::get_monitor_name());
            title->set_font_size(28);
            title->set_fixed_size(LABEL_HEIGHT);
            title->set_font_weight(FONT_WEIGHT_BOLD);

            auto version = std::make_unique<Label>(SystemInfo::get_monitor_resolution());
            version->set_fixed_size(LABEL_HEIGHT);

            auto gpu = std::make_unique<Label>(SystemInfo::get_graphics());
            gpu->set_font_size(18);
            gpu->set_fixed_size(LABEL_HEIGHT);
            gpu->set_font_weight(FONT_WEIGHT_BOLD);

            content->add_child(std::move(title));
            content->add_child(std::move(version));
            content->add_child(std::move(gpu));

            info_container->add_child(Spacer());
            info_container->add_child(std::move(content));
            info_container->add_child(Spacer());

            add_child(std::move(icon_container));
            add_child(std::move(info_container));
            add_child(std::move(btn_container));
        }
        ~Displays() = default;
    };
} // namespace horizon