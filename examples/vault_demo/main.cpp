#include "horizon/Widget.hpp"
#include <horizon/CategorizedBar.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Vault.hpp>
#include <horizon/WaylandWindow.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    WaylandWindow window("horizon.vault_demo", 400, 300);
    window.set_name("Vault Demo");

    auto root = std::make_unique<Widget>();
    root->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
    root->set_margin(20);
    root->set_spacing(10);

    auto toolbar = std::make_unique<Toolbar>("MainToolbar");

    toolbar->add_child(Spacer());

    auto search_btn = std::make_unique<ToolbarButton>("Search", "edit-find-symbolic");
    search_btn->set_fixed_size(60);
    toolbar->add_child(std::move(search_btn));

    auto edit_btn = std::make_unique<ToolbarButton>("Edit", "document-edit-symbolic");
    edit_btn->set_fixed_size(60);
    toolbar->add_child(std::move(edit_btn));

    auto gear_btn = std::make_unique<ToolbarButton>("Settings", "settings-gear-symbolic");
    gear_btn->set_fixed_size(60);

    // 1. Create the Vault
    auto vault = std::make_unique<Vault>();

    // 2. Create the content for the vault
    auto vault_content = std::make_unique<Widget>();
    vault_content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    vault_content->set_spacing(15);
    vault_content->set_width(300); // Base width

    auto title = std::make_unique<Label>("Vault Settings");
    title->set_font_weight(FONT_WEIGHT_BOLD);
    vault_content->add_child(std::move(title));

    auto slider_label = std::make_unique<Label>("Brightness Control");
    vault_content->add_child(std::move(slider_label));

    auto slider = std::make_unique<Slider>();
    vault_content->add_child(std::move(slider));

    auto search = std::make_unique<TextBox<>>();
    search->set_placeholder("Quick search...");
    vault_content->add_child(std::move(search));

    // 3. Put content into vault
    vault->set_content(std::move(vault_content));

    // 4. Associate vault with the button
    gear_btn->set_vault(std::move(vault));

    toolbar->add_child(std::move(gear_btn));

    auto help_btn = std::make_unique<ToolbarButton>("Help", "help-browser-symbolic");
    help_btn->set_fixed_size(60);
    toolbar->add_child(std::move(help_btn));

    root->add_child(std::move(toolbar));

    auto main_label = std::make_unique<Label>("Click the gear icon to open the Vault!");
    main_label->set_alignment(TextAlignment::Center);
    root->add_child(std::move(main_label));

    window.set_root(std::move(root));
    window.run();

    return 0;
}
