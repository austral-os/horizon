#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Combo.hpp>

namespace horizon::preferences
{
    class RegionView : public Widget
    {
    public:
        RegionView();
        ~RegionView() override = default;
    private:
        void load_languages();
        void load_formats();
        void load_timezones();

        Label* m_title_label{nullptr};
        Combo* m_lang_combo{nullptr};
        Combo* m_formats_combo{nullptr};
        Combo* m_timezone_combo{nullptr};
    };
}
