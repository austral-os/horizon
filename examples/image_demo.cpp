#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Image.hpp>
#include <horizon/Label.hpp>
#include <horizon/Panel.hpp>
#include <horizon/VPanel.hpp>

using namespace horizon;

int main(int argc, char *argv[])
{
    Application app(800, 600);

    auto window = std::make_unique<ApplicationWindow>("Image Widget Demo");

    auto root_panel = std::make_unique<VPanel>();
    root_panel->set_spacing(10);
    root_panel->set_margin(10);

    // Modes Demo
    auto add_demo = [&](const std::string &title, ImageMode mode, const std::string &path)
    {
        auto row = std::make_unique<Panel>();
        row->set_height(120);

        auto label = std::make_unique<Label>();
        label->set_text(title);
        label->set_width(100);

        auto img = std::make_unique<Image>();
        img->set_path(path);
        img->set_mode(mode);
        // Let it take remaining space

        row->add_child(std::move(label));
        row->add_child(std::move(img));
        root_panel->add_child(std::move(row));
    };

    // Note: You should have these files or change the paths
    std::string test_img = "/usr/share/pixmaps/antigravity.png";
    // If not found, it will just show empty.

    add_demo("Normal", ImageMode::Normal, test_img);
    add_demo("Stretch", ImageMode::Stretch, test_img);
    add_demo("Fit", ImageMode::Fit, test_img);
    add_demo("Repeat", ImageMode::Repeat, test_img);

    window->add_child(std::move(root_panel));
    app.set_root(std::move(window));

    app.run();
    return 0;
}
