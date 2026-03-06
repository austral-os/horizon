#pragma once

#include <functional>
#include <horizon/ScrollArea.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    /**
     * @class IconViewBase
     * @brief Base class for IconView to hold non-templated logic and state.
     */
    class IconViewBase : public Widget
    {
    public:
        IconViewBase();
        virtual ~IconViewBase() = default;

        void set_zoom(float zoom);
        float zoom() const;

        void set_selected_index(int index);
        int selected_index() const;

        void set_side_margin(int margin);
        int side_margin() const;

        void set_item_size(int width, int height);

        int get_theme_font_size(const std::string &role = "icon-view") const;

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

    protected:
        virtual void rebuild_items() = 0;

        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_pane{nullptr};

        float m_zoom{1.0f};
        int m_selected_index{-1};

        int m_item_width{80};
        int m_item_height{90};
        int m_side_margin{16};
        int m_grid_spacing{12};

        int BASE_ITEM_WIDTH{80};
        int BASE_ITEM_HEIGHT{80};
        int BASE_GRID_SPACING{12};
    };

    /**
     * @class IconView
     * @brief A generic widget that displays a grid of items from a given collection.
     */
    template <typename T> class IconView : public IconViewBase
    {
    public:
        using ItemFactory =
            std::function<std::unique_ptr<Widget>(const T &, float zoom, bool selected)>;

        IconView() : IconViewBase() {}
        ~IconView() override = default;

        std::function<void(int, const T &)> on_item_selected;

        void set_data(std::vector<T> data)
        {
            m_data = std::move(data);
            m_selected_index = -1;
            rebuild_items();
        }

        const std::vector<T> &data() const
        {
            return m_data;
        }

        void set_item_factory(ItemFactory factory)
        {
            m_item_factory = factory;
            rebuild_items();
        }

        void refresh()
        {
            rebuild_items();
        }

    protected:
        void rebuild_items() override
        {
            if (!m_content_pane)
                return;

            m_content_pane->clear_children();

            if (!m_item_factory)
                return;

            for (int i = 0; i < (int)m_data.size(); ++i)
            {
                bool is_selected = (i == m_selected_index);
                auto item_widget = m_item_factory(m_data[i], m_zoom, is_selected);
                if (item_widget)
                {
                    m_content_pane->add_child(std::move(item_widget));
                }
            }

            invalidate();
            calculate_layout();
        }

    private:
        std::vector<T> m_data;
        ItemFactory m_item_factory;
    };
} // namespace horizon
