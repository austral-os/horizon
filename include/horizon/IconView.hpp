#pragma once

#include <chrono>
#include <functional>
#include <horizon/ScrollArea.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Menu.hpp>
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

        void set_transparent(bool transparent);

        void set_selected_index(int index);
        int selected_index() const;

        void set_side_margin(int margin);
        int side_margin() const;

        void set_item_size(int width, int height);

        int get_theme_font_size(const std::string &role = "icon-view") const;

        void calculate_layout() override;

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

        std::chrono::steady_clock::time_point m_last_item_click_time;
        int m_last_item_click_index{-1};
        uint32_t m_last_item_click_button{0};
    };

    /**
     * @brief Context for an icon view item click event.
     */
    template <typename T> class IconViewItemMouseClickContext : public EventContext
    {
    public:
        int item_index;
        T item_data;
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

        EventsManager<IconViewItemMouseClickContext<T>> when_item_click;
        EventsManager<IconViewItemMouseClickContext<T>> when_item_dbl_click;

        std::function<void(int, const T &)> on_item_selected;

        void set_data(std::vector<T> data)
        {
            m_data = std::move(data);
            m_selected_index = -1;
            // Increment generation so any pending async rebuild is cancelled
            ++m_rebuild_generation;
            rebuild_items();
        }

        const std::vector<T> &data() const
        {
            return m_data;
        }

        /**
         * @brief Returns the items currently selected in the icon view.
         * @return A vector of data items of type T.
         */
        std::vector<T> get_selected_items() const
        {
            std::vector<T> selected_items;
            if (m_selected_index >= 0 && (size_t)m_selected_index < m_data.size())
            {
                selected_items.push_back(m_data[m_selected_index]);
            }
            return selected_items;
        }

        void set_item_factory(ItemFactory factory)
        {
            m_item_factory = factory;
            rebuild_items();
        }

        void set_item_menu_factory(std::function<std::unique_ptr<Menu>(const T &)> factory)
        {
            m_item_menu_factory = factory;
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
                    item_widget->when_mouse_press.connect(
                        [this, i](MouseButtonEventContext &ctx)
                        {
                            if (ctx.button == 0x110) // Left click
                            {
                                auto now = std::chrono::steady_clock::now();
                                auto duration =
                                    std::chrono::duration_cast<std::chrono::milliseconds>(
                                        now - m_last_item_click_time)
                                        .count();

                                if (m_last_item_click_index == i &&
                                    m_last_item_click_button == ctx.button && duration < 500)
                                {
                                    IconViewItemMouseClickContext<T> click_ctx;
                                    click_ctx.item_index = i;
                                    click_ctx.item_data = m_data[i];
                                    when_item_dbl_click.run(click_ctx);
                                    m_last_item_click_index = -1; // Reset
                                }
                                else
                                {
                                    set_selected_index(i);

                                    IconViewItemMouseClickContext<T> click_ctx;
                                    click_ctx.item_index = i;
                                    click_ctx.item_data = m_data[i];
                                    when_item_click.run(click_ctx);

                                    if (on_item_selected)
                                        on_item_selected(i, m_data[i]);

                                    m_last_item_click_time = now;
                                    m_last_item_click_index = i;
                                    m_last_item_click_button = ctx.button;
                                }
                            }
                            else if (ctx.button == 0x111) // Right click
                            {
                                set_selected_index(i);
                            }
                        });

                    if (m_item_menu_factory)
                    {
                        item_widget->set_context_menu(m_item_menu_factory(m_data[i]));
                    }

                    item_widget->set_position_type(FREE);
                    m_content_pane->add_child(std::move(item_widget));
                }
            }

            invalidate();
            calculate_layout();
        }

    private:
        std::vector<T> m_data;
        ItemFactory m_item_factory;
        std::function<std::unique_ptr<Menu>(const T &)> m_item_menu_factory;
        uint64_t m_rebuild_generation{0};
    };
} // namespace horizon
