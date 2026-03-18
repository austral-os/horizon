#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    /**
     * @class DesktopView
     * @brief View for configuring desktop wallpaper and related settings.
     */
    class DesktopView : public Widget
    {
    public:
        DesktopView();
        ~DesktopView() override = default;

    protected:
        void update_layout();

    private:
        Label* m_title_label{nullptr};
    };
} // namespace horizon::preferences
