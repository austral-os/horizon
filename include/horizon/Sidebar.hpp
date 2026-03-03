#pragma once
#include <horizon/Widget.hpp>
#include <map>
#include <string>

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

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        std::map<std::string, Widget *> m_groups;
    };
} // namespace horizon
