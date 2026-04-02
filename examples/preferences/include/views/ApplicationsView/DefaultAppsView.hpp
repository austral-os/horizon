#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Combo.hpp>
#include <utils/DesktopManager.hpp>
#include <string>
#include <vector>

namespace horizon::preferences
{
    struct DefaultAppCategoryItem
    {
        std::string label;
        std::string mime_type;
        std::vector<std::string> related_mimes;
        Combo* combo;
    };

    struct DefaultAppCategory
    {
        std::string name;
        std::vector<DefaultAppCategoryItem> items;
    };

    class DefaultAppsView : public Widget
    {
    public:
        DefaultAppsView();

        void calculate_layout() override;

    private:
        void setup_ui();
        void add_category(const DefaultAppCategory& category);
        void populate_combo(Combo* combo, const std::string& mime_type);
        void on_app_selected(const std::string& mime_type, const std::vector<std::string>& related_mimes, const std::string& desktop_id);

        std::vector<DefaultAppCategory> m_categories;
    };
} // namespace horizon::preferences
