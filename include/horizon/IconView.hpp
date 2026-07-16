#pragma once

#include <chrono>
#include <cmath>
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

        void set_scroll_enabled(bool enabled);

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

    template <typename T> class IconView : public IconViewBase
    {
    public:
        using ItemFactory =
            std::function<std::unique_ptr<Widget>(const T &, float zoom, bool selected)>;

        IconView() : IconViewBase() {
            set_focusable(true);

            when_key_press.connect([this](KeyEventContext &ev) {
                if (m_data.empty()) return;

                int data_idx = selected_index();
                int current_idx = -1;
                for (size_t v = 0; v < m_visual_to_data.size(); ++v) {
                    if (m_visual_to_data[v] == data_idx) {
                        current_idx = v;
                        break;
                    }
                }

                int new_idx = current_idx;
                int cols = std::max(1, m_columns_count);
                int rows = std::max(1, m_rows_count);

                bool is_arrow = false;

                if (m_layout_mode == IconViewLayoutMode::Horizontal) {
                    if (ev.keysym == 0xff51) // Left arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx - 1;
                        while (new_idx >= 0 && m_visual_to_data[new_idx] == -1) new_idx--;
                        if (new_idx < 0) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff53) // Right arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx + 1;
                        while (new_idx < (int)m_visual_to_data.size() && m_visual_to_data[new_idx] == -1) new_idx++;
                        if (new_idx >= (int)m_visual_to_data.size()) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff52) // Up arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx - cols;
                        while (new_idx >= 0 && m_visual_to_data[new_idx] == -1) new_idx--;
                        if (new_idx < 0) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff54) // Down arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx + cols;
                        while (new_idx < (int)m_visual_to_data.size() && m_visual_to_data[new_idx] == -1) new_idx++;
                        if (new_idx >= (int)m_visual_to_data.size()) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                } else {
                    if (ev.keysym == 0xff52) // Up arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx - 1;
                        while (new_idx >= 0 && m_visual_to_data[new_idx] == -1) new_idx--;
                        if (new_idx < 0) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff54) // Down arrow
                    {
                        this->set_focus(true);
                        new_idx = current_idx + 1;
                        while (new_idx < (int)m_visual_to_data.size() && m_visual_to_data[new_idx] == -1) new_idx++;
                        if (new_idx >= (int)m_visual_to_data.size()) new_idx = current_idx;
                        ev.stop_propagation = true;
                        is_arrow = true;
                    }
                    else if (ev.keysym == 0xff51 || ev.keysym == 0xff53) // Left or Right arrow
                    {
                        this->set_focus(true);
                        ev.stop_propagation = true;
                        is_arrow = true;

                        if (current_idx >= 0 && m_content_pane && current_idx < (int)m_content_pane->children().size()) {
                            bool move_left = (ev.keysym == 0xff51);
                            auto cx = m_content_pane->children()[current_idx]->x();
                            auto cy = m_content_pane->children()[current_idx]->y() + m_content_pane->children()[current_idx]->height() / 2;

                            int min_dx = 1000000;
                            for (int i = 0; i < (int)m_content_pane->children().size(); ++i) {
                                if (i == current_idx || m_visual_to_data[i] == -1) continue;
                                int dx = m_content_pane->children()[i]->x() - cx;
                                if ((move_left && dx < 0) || (!move_left && dx > 0)) {
                                    int abs_dx = std::abs(dx);
                                    if (abs_dx < min_dx) {
                                        min_dx = abs_dx;
                                    }
                                }
                            }

                            if (min_dx != 1000000) {
                                int min_dy = 1000000;
                                int best_idx = current_idx;
                                for (int i = 0; i < (int)m_content_pane->children().size(); ++i) {
                                    if (i == current_idx || m_visual_to_data[i] == -1) continue;
                                    int dx = m_content_pane->children()[i]->x() - cx;
                                    if ((move_left && dx < 0) || (!move_left && dx > 0)) {
                                        if (std::abs(dx) <= min_dx + 5) {
                                            int child_cy = m_content_pane->children()[i]->y() + m_content_pane->children()[i]->height() / 2;
                                            int dy = std::abs(child_cy - cy);
                                            if (dy < min_dy) {
                                                min_dy = dy;
                                                best_idx = i;
                                            }
                                        }
                                    }
                                }
                                new_idx = best_idx;
                            }
                        } else if (current_idx < 0 && m_data.size() > 0) {
                            new_idx = 0;
                            while (new_idx < (int)m_visual_to_data.size() && m_visual_to_data[new_idx] == -1) new_idx++;
                        }
                    }
                }

                if (new_idx != current_idx && new_idx >= 0 && new_idx < (int)m_visual_to_data.size())
                {
                    int target_data_idx = m_visual_to_data[new_idx];
                    if (target_data_idx != -1) {
                        bool shift_pressed = (ev.modifiers & WaylandWindow::Modifier::SHIFT);
                        bool ctrl_pressed = (ev.modifiers & WaylandWindow::Modifier::CTRL);
                        set_selected_index(target_data_idx, ctrl_pressed, shift_pressed);
                        
                        if (on_item_selected)
                            on_item_selected(target_data_idx, m_data[target_data_idx]);
                    }

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

        void set_grouped(bool grouped) {
            if (m_is_grouped != grouped) {
                m_is_grouped = grouped;
                if (!grouped) {
                    m_expanded_group = "";
                    m_animating_group = "";
                    m_collapsing_group = "";
                    m_is_group_animation_active = false;
                }
                // Before the widget is attached to a window, there is no event
                // loop to defer to. Initial setup can safely rebuild directly.
                if (!application()) {
                    rebuild_items();
                    return;
                }

                // Defer rebuild to avoid use-after-free when called from
                // popup menu event handlers — rebuild_items() → clear_children()
                // would destroy the widget tree while the event chain is active.
                ++m_rebuild_generation;
                auto gen = m_rebuild_generation;
                application()->add_timer(0, [this, gen]() {
                    if (gen == m_rebuild_generation)
                        rebuild_items();
                });
            }
        }
        bool is_grouped() const { return m_is_grouped; }

        void set_grouping_function(std::function<std::string(const T&)> func) {
            m_grouping_function = func;
            if (m_is_grouped) rebuild_items();
        }

        void set_group_header_factory(std::function<std::unique_ptr<Widget>(const std::string&, bool, float)> factory) {
            m_group_header_factory = factory;
            if (m_is_grouped) rebuild_items();
        }

        void refresh()
        {
            rebuild_items();
        }

        void calculate_layout() override
        {
            IconViewBase::calculate_layout();
            apply_group_animation_layout();
        }

    protected:
        void schedule_rebuild()
        {
            ++m_rebuild_generation;
            auto gen = m_rebuild_generation;

            if (!application()) {
                if (gen == m_rebuild_generation)
                    rebuild_items();
                return;
            }

            application()->add_timer(0, [this, gen]() {
                if (gen == m_rebuild_generation)
                    rebuild_items();
            });
        }

        void start_group_animation(const std::string &group_name, bool expanding)
        {
            if (!application()) {
                rebuild_items();
                return;
            }

            ++m_group_animation_generation;
            auto gen = m_group_animation_generation;

            m_animating_group = group_name;
            m_collapsing_group = expanding ? "" : group_name;
            m_is_group_animation_expanding = expanding;
            m_is_group_animation_active = true;
            m_group_animation_progress = expanding ? 0.0f : 1.0f;

            schedule_rebuild();

            auto start = std::chrono::steady_clock::now();
            start_group_animation_frame(gen, start);
        }

        void start_group_animation_frame(uint64_t gen, std::chrono::steady_clock::time_point start)
        {
            constexpr int duration_ms = 160;
            constexpr int frame_ms = 16;

            application()->add_timer(frame_ms, [this, gen, start]() {
                if (gen != m_group_animation_generation)
                    return;

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                float t = std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(duration_ms));
                float eased = 1.0f - std::pow(1.0f - t, 3.0f);

                m_group_animation_progress = m_is_group_animation_expanding ? eased : 1.0f - eased;
                calculate_layout();
                invalidate();

                if (t >= 1.0f) {
                    m_is_group_animation_active = false;
                    m_animating_group = "";

                    if (!m_is_group_animation_expanding) {
                        finish_group_collapse();
                    } else {
                        m_collapsing_group = "";
                    }
                    return;
                }

                if (application())
                    start_group_animation_frame(gen, start);
            });
        }

        void finish_group_collapse()
        {
            if (!m_content_pane || m_collapsing_group.empty()) {
                m_collapsing_group = "";
                m_expanded_group = "";
                return;
            }

            std::set<const Widget *> widgets_to_remove;
            auto &children = m_content_pane->children();

            for (size_t i = 0; i < children.size() && i < m_visual_to_group.size() && i < m_visual_to_data.size(); ++i) {
                if (m_visual_to_group[i] == m_collapsing_group && m_visual_to_data[i] != -1) {
                    widgets_to_remove.insert(children[i].get());
                }
            }

            if (!widgets_to_remove.empty()) {
                m_content_pane->remove_children_if([&widgets_to_remove](const Widget *child) {
                    return widgets_to_remove.count(child) > 0;
                });

                for (int i = static_cast<int>(m_visual_to_data.size()) - 1; i >= 0; --i) {
                    if (i < static_cast<int>(m_visual_to_group.size()) &&
                        m_visual_to_group[i] == m_collapsing_group && m_visual_to_data[i] != -1) {
                        m_visual_to_data.erase(m_visual_to_data.begin() + i);
                        m_visual_to_group.erase(m_visual_to_group.begin() + i);
                    }
                }
            }

            m_collapsing_group = "";
            m_expanded_group = "";
            calculate_layout();
            invalidate();
        }

        void apply_group_animation_layout()
        {
            if (!m_is_group_animation_active || m_animating_group.empty() || !m_content_pane)
                return;

            auto &children = m_content_pane->children();
            if (children.size() != m_visual_to_group.size() || children.size() != m_visual_to_data.size())
                return;

            int anchor_y = 0;
            bool found_anchor = false;

            for (size_t i = 0; i < children.size(); ++i) {
                if (m_visual_to_group[i] == m_animating_group && m_visual_to_data[i] == -1) {
                    anchor_y = children[i]->y();
                    found_anchor = true;
                    break;
                }
            }

            if (!found_anchor)
                return;

            float progress = std::max(0.0f, std::min(1.0f, m_group_animation_progress));
            for (size_t i = 0; i < children.size(); ++i) {
                if (m_visual_to_group[i] != m_animating_group || m_visual_to_data[i] == -1)
                    continue;

                int target_y = children[i]->y();
                int animated_y = anchor_y + static_cast<int>((target_y - anchor_y) * progress);
                children[i]->set_position(children[i]->x(), animated_y);
            }
        }

        void rebuild_items() override
        {
            if (!m_content_pane)
                return;

            m_content_pane->clear_children();
            m_visual_to_data.clear();
            m_visual_to_group.clear();

            if (!m_item_factory)
                return;

            if (m_is_grouped && m_grouping_function && m_group_header_factory) {
                std::map<std::string, std::vector<int>> groups;
                for (int i = 0; i < (int)m_data.size(); ++i) {
                    groups[m_grouping_function(m_data[i])].push_back(i);
                }

                for (const auto& pair : groups) {
                    std::string group_name = pair.first;
                    const auto& indices = pair.second;
                    
                    bool is_expanded = (m_expanded_group == group_name);
                    bool is_collapsing = (m_collapsing_group == group_name);
                    auto header = m_group_header_factory(group_name, is_expanded && !is_collapsing, m_zoom);
                    if (header) {
                        header->when_mouse_press.connect([](MouseButtonEventContext &ctx) {
                            ctx.stop_propagation = true;
                        });
                        std::string gname = group_name;
                        header->when_click.connect([this, gname](MouseButtonEventContext &ctx) {
                            bool expanding = (m_expanded_group != gname);
                            if (expanding)
                                m_expanded_group = gname;

                            // Defer and animate rebuild to avoid self-destruction during callback.
                            start_group_animation(gname, expanding);
                            ctx.stop_propagation = true;
                        });
                        header->set_position_type(FREE);
                        m_content_pane->add_child(std::move(header));
                        m_visual_to_data.push_back(-1);
                        m_visual_to_group.push_back(group_name);
                    }

                    if (is_expanded || is_collapsing) {
                        for (int i : indices) {
                            add_item_widget(i, group_name);
                        }
                    }
                }
            } else {
                for (int i = 0; i < (int)m_data.size(); ++i)
                {
                    add_item_widget(i, "");
                }
            }

            invalidate();
            calculate_layout();
        }

        void add_item_widget(int i, const std::string &group_name)
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
                            
                            this->m_active_item_menu = m_item_menu_factory(item_data);
                            if (this->application() && this->m_active_item_menu) {
                                this->application()->show_context_menu(this->m_active_item_menu.get(), ctx.x, ctx.y, ctx.serial, this);
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
                    m_visual_to_data.push_back(i);
                    m_visual_to_group.push_back(group_name);
                }
        }

    private:
        std::vector<T> m_data;
        ItemFactory m_item_factory;
        std::function<std::unique_ptr<Menu>(const T &)> m_item_menu_factory;
        uint64_t m_rebuild_generation{0};
        
        bool m_is_grouped{false};
        std::string m_expanded_group{""};
        std::function<std::string(const T&)> m_grouping_function;
        std::function<std::unique_ptr<Widget>(const std::string&, bool, float)> m_group_header_factory;
        std::vector<int> m_visual_to_data;
        std::vector<std::string> m_visual_to_group;
        uint64_t m_group_animation_generation{0};
        bool m_is_group_animation_active{false};
        bool m_is_group_animation_expanding{false};
        float m_group_animation_progress{1.0f};
        std::string m_animating_group;
        std::string m_collapsing_group;
        std::unique_ptr<Menu> m_active_item_menu;
    };
} // namespace horizon
