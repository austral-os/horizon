#include <horizon/Application.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/AvatarSelector.hpp>
#include <horizon/Label.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;

int main(int argc, char** argv)
{
    Application app("avatar-selector-demo");
    
    // Set the local avatars directory for the demo
#ifdef HORIZON_SOURCE_DIR
    AvatarSelector::set_avatars_directory(std::string(HORIZON_SOURCE_DIR) + "/apps/horizon_session/data/avatars");
#endif

    WaylandWindow window("Avatar Selector Demo", 600, 400);

    auto root = std::make_unique<Widget>();
    root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    root->set_margin(40);
    root->set_spacing(20);

    auto title = std::make_unique<Label>("Avatar Selector Demo");
    title->set_font_size(24);
    title->set_font_weight(FONT_WEIGHT_BOLD);
    title->set_alignment(TextAlignment::Center);
    root->add_child(std::move(title));

    auto description = std::make_unique<Label>("Click the circle below to select your avatar.");
    description->set_alignment(TextAlignment::Center);
    root->add_child(std::move(description));

    // Create the AvatarSelector
    auto selector = std::make_unique<AvatarSelector>();
    selector->set_fixed_size(128); // Make it nice and big
    
    // Pointer to the selector to use in the lambda
    auto selector_ptr = selector.get();

    auto status_label = std::make_unique<Label>("Current selection: avatar-default");
    status_label->set_alignment(TextAlignment::Center);
    auto status_label_ptr = status_label.get();

    // Connect to the selection changed event
    selector->when_selection_changed.connect([selector_ptr, status_label_ptr](EventContext& ctx) {
        std::string selected = selector_ptr->selected_avatar();
        LOG_INFO << "Selection changed: " << selected;
        status_label_ptr->set_text("Current selection: " + selected);
    });

    // Center the selector in the layout
    auto center_box = std::make_unique<Widget>();
    center_box->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    center_box->add_child(std::move(selector));
    root->add_child(std::move(center_box));

    root->add_child(std::move(status_label));

    window.set_root(std::move(root));
    window.run();

    return 0;
}
