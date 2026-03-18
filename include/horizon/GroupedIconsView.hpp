#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <string>
#include <vector>

namespace horizon
{
    /**
     * @struct GroupedIconItem
     * @brief Represents a single item in a GroupedIconsView.
     */
    struct GroupedIconItem
    {
        std::string id;
        std::string label;
        std::string icon_name;
    };

    /**
     * @struct IconGroup
     * @brief Represents a group of items with a title.
     */
    struct IconGroup
    {
        std::string title;
        std::vector<GroupedIconItem> items;
    };

    /**
     * @class GroupedIconsView
     * @brief A widget that displays icons grouped under headers, similar to macOS System Preferences.
     */
    class GroupedIconsView : public Widget
    {
    public:
        GroupedIconsView();
        ~GroupedIconsView() override = default;

        /**
         * @brief Adds a group of icons to the view.
         */
        void add_group(const IconGroup &group);

        /**
         * @brief Clears all groups from the view.
         */
        void clear_groups();

        /**
         * @brief Sets the alternate background colors for groups.
         */
        void set_alternate_colors(const Color &c1, const Color &c2);

        void calculate_layout() override;

        EventsManager<const GroupedIconItem&> when_item_click;
        EventsManager<const GroupedIconItem&> when_item_dbl_click;

    private:
        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_pane{nullptr};
        std::vector<IconGroup> m_groups;
        Color m_alt_color1{0.0f, 0.0f, 0.0f, 0.0f};
        Color m_alt_color2{0.0f, 0.0f, 0.0f, 0.0f};
        bool m_has_alt_colors{false};

        void rebuild_ui();
    };

    /**
     * @internal
     * Helper widgets for the GroupedIconsView
     */

    class GroupIconItemWidget : public Widget
    {
    public:
        GroupIconItemWidget(GroupedIconsView* view, const GroupedIconItem &item);
        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

        int preferred_height(int width) const override;

    private:
        Icon *m_icon{nullptr};
        Label *m_label{nullptr};
        GroupedIconsView* m_view{nullptr};
        GroupedIconItem m_item_data;
    };

    class GroupGrid : public Widget
    {
    public:
        GroupGrid();
        void calculate_layout() override;
        int preferred_height(int width) const override;

    private:
        int m_item_width{100};
        int m_grid_spacing{10};
    };

    class GroupSeparator : public Widget
    {
    public:
        GroupSeparator();
        void draw(GraphicsContext &gc) override;
        int preferred_height(int width) const override;
    };

    class GroupContainer : public Widget
    {
    public:
        GroupContainer();
        void calculate_layout() override;
        int preferred_height(int width) const override;
    };

} // namespace horizon
