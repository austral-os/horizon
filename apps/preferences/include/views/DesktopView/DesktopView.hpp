#pragma once

#include <horizon/Notebook.hpp>
#include <horizon/Widget.hpp>
#include <views/DesktopView/DockView.hpp>
#include <views/DesktopView/WallpaperView.hpp>

namespace horizon::preferences
{
    /**
     * @class DesktopView
     * @brief Container view for desktop-related settings, using a Notebook for organization.
     */
    class DesktopView : public horizon::Widget
    {
    public:
        DesktopView();
        ~DesktopView() override = default;

    private:
        horizon::Notebook *m_notebook{nullptr};
        WallpaperView *m_wallpaper_view{nullptr};
        DockView *m_dock_view{nullptr};
    };
} // namespace horizon::preferences
