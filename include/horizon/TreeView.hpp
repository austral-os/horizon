#pragma once
#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/TreeViewItem.hpp>

namespace horizon
{
    /**
     * @class TreeView
     * @brief A widget that displays a tree of items with scrollbars.
     */
    class TreeView : public Widget
    {
    public:
        TreeView();
        virtual ~TreeView() = default;

        void add_root_item(std::unique_ptr<TreeViewItem> item);

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

    private:
        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_container{nullptr};
    };
} // namespace horizon
