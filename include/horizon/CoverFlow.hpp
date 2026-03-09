#pragma once

#include <functional>
#include <horizon/Widget.hpp>
#include <map>
#include <memory>
#include <vector>

namespace horizon
{
    /**
     * @class CoverFlowBase
     * @brief Base class for CoverFlow to hold non-templated logic and state.
     */
    class CoverFlowBase : public Widget
    {
    public:
        CoverFlowBase();
        virtual ~CoverFlowBase();

        void set_selected_index(int index);
        int selected_index() const;
        void update_animation();

        void set_item_size(int width, int height);

        void set_draw_reflection(bool draw)
        {
            m_draw_reflection = draw;
            invalidate();
        }
        bool draw_reflection() const
        {
            return m_draw_reflection;
        }

        void set_dark_mode(bool dark)
        {
            m_dark_mode = dark;
            invalidate();
        }
        bool dark_mode() const
        {
            return m_dark_mode;
        }

        void calculate_layout() override;
        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;

        /**
         * @brief Override hit test to handle overlapping items properly.
         */
        Widget *hit_test(int x, int y) override;

    protected:
        virtual void rebuild_items() = 0;

        int m_selected_index{-1};
        float m_animated_index{-1.0f};
        size_t m_animation_timer{0};

        int m_item_width{190};
        int m_item_height{190};

        bool m_draw_reflection{true};
        bool m_dark_mode{true};

        // Interaction state
        bool m_is_dragging{false};
        int m_mouse_press_x{0};
        float m_drag_start_animated_index{0.0f};

        struct CachedTexture
        {
            uint32_t texture_id{0};
            int width{0};
            int height{0};
        };

        std::map<Widget *, CachedTexture> m_texture_cache;
        void clear_cache();
    };

    /**
     * @class CoverFlow
     * @brief A generic widget that displays items in a 3D-like cover flow.
     */
    template <typename T> class CoverFlow : public CoverFlowBase
    {
    public:
        using ItemFactory = std::function<std::unique_ptr<Widget>(const T &, bool selected)>;

        CoverFlow() : CoverFlowBase() {}
        ~CoverFlow() override = default;

        EventsManager<EventContext> when_selection_changed;

        void set_data(std::vector<T> data)
        {
            m_data = std::move(data);
            m_selected_index = m_data.empty() ? -1 : 0;
            m_animated_index = (float)m_selected_index;
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
            clear_cache();
            clear_children();

            if (!m_item_factory)
                return;

            for (int i = 0; i < (int)m_data.size(); ++i)
            {
                bool is_selected = (i == m_selected_index);
                auto item_widget = m_item_factory(m_data[i], is_selected);
                if (item_widget)
                {
                    item_widget->set_position_type(WidgetPositionTypes::FREE);

                    // Allow clicking on any item to select it
                    item_widget->when_mouse_press.connect(
                        [this, i](MouseButtonEventContext &ev)
                        {
                            if (ev.button == 0x110) // Left click
                            {
                                set_selected_index(i);
                                EventContext ctx;
                                when_selection_changed.run(ctx);
                            }
                        });

                    add_child(std::move(item_widget));
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
