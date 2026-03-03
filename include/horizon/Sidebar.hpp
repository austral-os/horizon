#pragma once
#include <horizon/ScrollArea.hpp>
#include <horizon/Widget.hpp>
#include <map>
#include <memory>

namespace horizon
{
    /**
     * @class Sidebar
     * @brief A navigation widget that organizes items into groups.
     */
    class Sidebar : public Widget
    {
    public:
        Sidebar();
        ~Sidebar() = default;

        /**
         * @brief Adds a new group header to the sidebar.
         * @param name The name of the group.
         */
        void add_group(const std::string &name);

        /**
         * @brief Adds a widget item to a specific group.
         * @param group_name The name of the group to add the item to.
         * @param item The widget to add.
         */
        void add_item(const std::string &group_name, std::unique_ptr<Widget> item);

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        void calculate_internal_layout();
        std::map<std::string, Widget *> m_groups;
        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_container{nullptr};
    };
} // namespace horizon
