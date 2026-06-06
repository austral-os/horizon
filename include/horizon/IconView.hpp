#pragma once

#include <chrono>
#include <functional>
#include <horizon/ScrollArea.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Menu.hpp>
#include <memory>
#include <set>
#include <vector>
namespace horizon
{
    enum class IconViewLayoutMode {
        Horizontal,
        VerticalLeftToRight,
        VerticalRightToLeft
    };

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

        void set_selected_index(int index, bool ctrl = false, bool shift = false);
        int selected_index() const;
        void clear_selection();

        void set_side_margin(int margin);
        int side_margin() const;

        void set_layout_mode(IconViewLayoutMode mode);
        IconViewLayoutMode layout_mode() const;

        void set_item_size(int width, int height);

        int get_theme_font_size(const std::string &role = "icon-view") const;

        void set_rubberband_selection_enabled(bool enabled);
        bool rubberband_selection_enabled() const;

        void draw(GraphicsContext &gc) override;

        void calculate_layout() override;
        void set_application_recursive(WaylandWindow *app) override;

    protected:
        virtual void rebuild_items() = 0;

        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_pane{nullptr};

        float m_zoom{1.0f};
        int m_selected_index{-1}; // kept for backward-compat (first in set)
        std::set<int> m_selected_indices;

        int m_item_width{80};
        int m_item_height{90};
        int m_side_margin{16};
        int m_grid_spacing{12};

        int BASE_ITEM_WIDTH{80};
        int BASE_ITEM_HEIGHT{80};
        int BASE_GRID_SPACING{12};

        int m_columns_count{1};
        int m_rows_count{1};

        std::chrono::steady_clock::time_point m_last_item_click_time;
        int m_last_item_click_index{-1};
        uint32_t m_last_item_click_button{0};

        bool m_transparent{false};
        IconViewLayoutMode m_layout_mode{IconViewLayoutMode::Horizontal};

        bool m_rubberband_selection_enabled{false};
        bool m_is_rubberbanding{false};
        int m_rubberband_start_x{0};
        int m_rubberband_start_y{0};
        int m_rubberband_current_x{0};
        int m_rubberband_current_y{0};
        std::set<int> m_initial_selection;
        uint32_t m_autoscroll_timer{0};
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

        IconView() : IconViewBase() {
            set_focusable(true);

            when_key_press.connect([this](KeyEventContext &ev) {
                if (m_data.empty()) return;

                int current_idx = selected_index();
                int new_idx = current_idx;
                int cols = std::max(1, m_columns_count);
                int rows = std::max(1, m_rows_count);

                bool is_arrow = false;

                if (m_layout_mode == IconViewLayoutMode::Horizontal) {
                    if (ev.keysym == 0xff51) // Left arrow
                    {
                        this->set_focus(true);
                        new_idx = (current_idx <= 0) ? 0 : current_idx - 1;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff53) // Right arrow
                    {
                        this->set_focus(true);
                        new_idx = (current_idx < 0) ? 0 : std::min((int)m_data.size() - 1, current_idx + 1);
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff52) // Up arrow
                    {
                        this->set_focus(true);
                        if (current_idx < 0) new_idx = 0;
                        else new_idx = std::max(0, current_idx - cols);
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff54) // Down arrow
                    {
                        this->set_focus(true);
                        if (current_idx < 0) new_idx = 0;
                        else new_idx = std::min((int)m_data.size() - 1, current_idx + cols);
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                } else {
                    if (ev.keysym == 0xff52) // Up arrow
                    {
                        this->set_focus(true);
                        new_idx = (current_idx <= 0) ? 0 : current_idx - 1;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff54) // Down arrow
                    {
                        this->set_focus(true);
                        new_idx = (current_idx < 0) ? 0 : std::min((int)m_data.size() - 1, current_idx + 1);
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff51) // Left arrow
                    {
                        this->set_focus(true);
                        if (m_layout_mode == IconViewLayoutMode::VerticalRightToLeft) {
                            new_idx = (current_idx < 0) ? 0 : std::min((int)m_data.size() - 1, current_idx + rows);
                        } else {
                            if (current_idx < 0) new_idx = 0;
                            else new_idx = std::max(0, current_idx - rows);
                        }
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff53) // Right arrow
                    {
                        this->set_focus(true);
                        if (m_layout_mode == IconViewLayoutMode::VerticalRightToLeft) {
                            if (current_idx < 0) new_idx = 0;
                            else new_idx = std::max(0, current_idx - rows);
                        } else {
                            new_idx = (current_idx < 0) ? 0 : std::min((int)m_data.size() - 1, current_idx + rows);
                        }
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                }

                if (new_idx != current_idx && new_idx >= 0 && new_idx < (int)m_data.size())
                {
                    bool shift_pressed = (ev.modifiers & WaylandWindow::Modifier::SHIFT);
                    bool ctrl_pressed = (ev.modifiers & WaylandWindow::Modifier::CTRL);
                    set_selected_index(new_idx, ctrl_pressed, shift_pressed);
                    
                    if (on_item_selected)
                        on_item_selected(new_idx, m_data[new_idx]);

                    // Auto-scroll logic:
                    if (m_scroll_area && m_content_pane && new_idx < (int)m_content_pane->children().size()) {
                        Widget* item = m_content_pane->children()[new_idx].get();
                        int item_y = item->y() - m_y + m_scroll_area->scroll_y();
                        int item_h = item->height();
                        int current_scroll_y = m_scroll_area->scroll_y();
                        int viewport_h = m_scroll_area->height();

                        if (item_y < current_scroll_y) {
                            m_scroll_area->set_scroll_position(m_scroll_area->scroll_x(), item_y);
                        } else if (item_y + item_h > current_scroll_y + viewport_h) {
                            m_scroll_area->set_scroll_position(m_scroll_area->scroll_x(), item_y + item_h - viewport_h);
                        }
                    }
                }
            });
        }
        ~IconView() override = default;

        EventsManager<IconViewItemMouseClickContext<T>> when_item_click;
        EventsManager<IconViewItemMouseClickContext<T>> when_item_dbl_click;

        std::function<void(int, const T &)> on_item_selected;

        void set_data(std::vector<T> data)
        {
            m_data = std::move(data);
            m_selected_index = -1;
            m_selected_indices.clear();
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
            for (int idx : m_selected_indices)
            {
                if (idx >= 0 && (size_t)idx < m_data.size())
                    selected_items.push_back(m_data[idx]);
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
                bool is_selected = (m_selected_indices.count(i) > 0);
                auto item_widget = m_item_factory(m_data[i], m_zoom, is_selected);
                if (item_widget)
                {
                    item_widget->when_mouse_press.connect([](MouseButtonEventContext &ctx) {
                        ctx.stop_propagation = true;
                    });

                    item_widget->when_click.connect([this, i](MouseButtonEventContext &ctx) {
                        bool ctrl_pressed = (ctx.modifiers & WaylandWindow::Modifier::CTRL);
                        bool shift_pressed = (ctx.modifiers & WaylandWindow::Modifier::SHIFT);

                        auto now = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            now - m_last_item_click_time)
                                            .count();

                        if (!ctrl_pressed && m_last_item_click_index == i &&
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
                            set_selected_index(i, ctrl_pressed, shift_pressed);

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
                        
                        ctx.stop_propagation = true;
                    });

                    // Keep when_dbl_click connected for API consistency, though it won't fire for unselected items due to rebuild_items
                    item_widget->when_dbl_click.connect([this, i](MouseButtonEventContext &ctx) {
                        IconViewItemMouseClickContext<T> click_ctx;
                        click_ctx.item_index = i;
                        click_ctx.item_data = m_data[i];
                        when_item_dbl_click.run(click_ctx);
                        ctx.stop_propagation = true;
                    });

                    if (m_item_menu_factory)
                    {
                        T item_data = m_data[i];
                        item_widget->when_right_click.connect([this, item_data, i](auto &ctx) {
                            if (m_selected_indices.count(i) == 0)
                                set_selected_index(i, false, false);
                            
                            this->set_context_menu(m_item_menu_factory(item_data));
                            if (this->application() && this->context_menu()) {
                                this->application()->show_context_menu(this->context_menu(), -1, -1, ctx.serial, this);
                            }
                            ctx.stop_propagation = true;
                        });
                    }
                    else
                    {
                        item_widget->when_right_click.connect([this, i](auto &ctx) {
                            if (m_selected_indices.count(i) == 0)
                                set_selected_index(i, false, false);
                        });
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
