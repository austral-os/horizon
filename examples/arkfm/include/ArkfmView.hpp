#pragma once

#include "horizon/Widget.hpp"

namespace horizon::arkfm
{

    enum class ViewMode
    {
        List,
        Grid,
        CoverFlow
    };

    class ArkfmView : public Widget
    {
    public:
        ArkfmView(std::string path = ".");
        ~ArkfmView() = default;

        void set_view_mode(ViewMode mode);

    private:
        ViewMode m_view_mode;
        std::string m_current_path;
    };

} // namespace horizon::arkfm