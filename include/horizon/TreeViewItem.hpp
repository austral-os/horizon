#pragma once
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon
{
    /**
     * @class TreeViewItem
     * @brief An item in a TreeView, supporting nested children, icons, and text.
     */
    class TreeViewItem : public Widget
    {
    public:
        TreeViewItem(const std::string &icon_name, const std::string &text);
        virtual ~TreeViewItem() = default;

        void set_expanded(bool expanded);
        bool is_expanded() const { return m_expanded; }

        void set_bold(bool bold);
        bool is_bold() const { return m_bold; }

        void add_item(std::unique_ptr<TreeViewItem> item);

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;
        Widget *hit_test(int x, int y) override;

        int total_height() const;

    private:
        std::string m_icon_name;
        std::string m_text;
        bool m_expanded{false};
        bool m_bold{false};
        int m_indentation_level{0};

        Widget *m_header{nullptr};
        Widget *m_spacer{nullptr};
        Widget *m_disclosure_container{nullptr};
        Icon *m_disclosure_icon{nullptr};
        Icon *m_item_icon{nullptr};
        Label *m_label{nullptr};

        void update_subitems_visibility();

        friend class TreeView;
    };
} // namespace horizon
