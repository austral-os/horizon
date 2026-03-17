#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Logger.hpp>

using horizon::WaylandWindow;
using horizon::Window;
using horizon::Label;
using horizon::VPanel;

int main()
{
    try
    {
        // Initial size 800x600, min size 400x300
        WaylandWindow app("horizon.min_size_test", 800, 600, false, true, 400, 300);
        app.set_name("Min Size Test");

        auto wnd = std::make_unique<Window>("Minimum Size Constraint Test");
        auto layout = std::make_unique<VPanel>();
        layout->set_margin(20);
        layout->set_spacing(10);

        auto lbl1 = std::make_unique<Label>("This window has a minimum size of 400x300.");
        auto lbl2 = std::make_unique<Label>("Try to resize it smaller than that.");
        
        layout->add_child(std::move(lbl1));
        layout->add_child(std::move(lbl2));
        wnd->add_child(std::move(layout));
        app.set_root(std::move(wnd));

        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
