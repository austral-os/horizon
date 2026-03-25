#include <views/DesktopView/DesktopView.hpp>
#include <memory>

namespace horizon::preferences
{
    DesktopView::DesktopView() : horizon::Widget()
    {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::WidgetPositionTypes::FILL);
        set_margin(0); // Notebook handles its own internal spacing/margin if needed
        set_spacing(0);

        auto notebook = std::make_unique<horizon::Notebook>();
        m_notebook = notebook.get();

        // --- Tab 1: Wallpaper ---
        auto wallpaper_view = std::make_unique<WallpaperView>();
        m_wallpaper_view = wallpaper_view.get();
        m_notebook->add_tab(horizon::NotebookPage("Escritorio", "preferences-desktop-wallpaper", std::move(wallpaper_view)));

        // --- Tab 2: Dock ---
        auto dock_view = std::make_unique<DockView>();
        m_dock_view = dock_view.get();
        m_notebook->add_tab(horizon::NotebookPage("Dock", "preferences-system", std::move(dock_view)));

        add_child(std::move(notebook));
    }
} // namespace horizon::preferences
