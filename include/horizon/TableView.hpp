#pragma once

#include <algorithm>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/TableRow.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    /**
     * @enum TableViewWidthMode
     * @brief Determines how the TableView calculates its total width.
     */
    enum class TableViewWidthMode
    {
        Fill,     // Occupy all horizontal space from parent
        Unbounded // Width is the sum of column widths, enabling horizontal scroll if needed
    };

    /**
     * @class TableView
     * @brief A template-based widget for displaying data in a grid.
     */
    template <typename T> class TableView : public Widget
    {
    public:
        TableView() : Widget()
        {
            m_layout_type = WIDGET_LAYOUT_VERTICAL;
            m_position_type = FILL;

            // Header Area
            m_header_container = std::make_unique<Widget>();
            m_header_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            m_header_container->set_position_type(FREE);
            m_header_container->set_spacing(0);
            m_header_container->set_margin(0);

            // Content Area (Scrollable)
            m_scroll_area = std::make_unique<ScrollArea>();
            m_scroll_area->set_position_type(FREE); // Critical: Manual layout

            m_content_container = std::make_unique<Widget>();
            m_content_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            m_content_container->set_position_type(FREE);
            m_content_container->set_spacing(0);
            m_content_container->set_margin(0);

            // Synchronize horizontal scroll
            m_scroll_area->when_scroll.connect([this](EventContext &) { sync_header_scroll(); });

            // Internal construction
            m_scroll_area->set_content(std::move(m_content_container));

            Widget::add_child(std::move(m_header_container));
            Widget::add_child(std::move(m_scroll_area));
        }

        virtual ~TableView() = default;

        void set_application_recursive(Application *app) override
        {
            Widget::set_application_recursive(app);
            rebuild_header();
            rebuild_content();
        }

        void add_column(TableColumn<T> col)
        {
            m_columns.push_back(std::move(col));
            rebuild_header();
            rebuild_content();
        }

        void set_data(std::vector<T> data)
        {
            m_data = std::move(data);
            rebuild_content();
        }

        void set_width_mode(TableViewWidthMode mode)
        {
            m_width_mode = mode;
            invalidate();
        }

        void set_alternate_colors(Color c1, Color c2)
        {
            // Keeping for backward compatibility but marking as potentially deprecated
            // if we want full themed enforcement.
            rebuild_content();
        }

        void set_header_visible(bool visible)
        {
            m_header_visible = visible;
            if (!children().empty())
                children()[0]->set_visible(visible);
            invalidate();
        }

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override
        {
            if (!m_visible)
                return;

            calculate_internal_layout();

            // Intersects check (dirty region)
            bool intersects =
                !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);

            if (!intersects)
                return;

            bool should_draw = m_dirty || force || m_child_dirty;
            if (should_draw)
                draw(gc);

            int header_h = m_header_visible ? 32 : 0;

            // Render children manually to apply clipping to header
            if (!children().empty())
            {
                if (m_header_visible)
                {
                    gc.save();
                    // Clip to table's horizontal bounds and header's vertical area
                    gc.clip(m_x, m_y, m_width, header_h);
                    children()[0]->render(gc, cx, cy, cw, ch, force);
                    gc.restore();
                }

                if (children().size() > 1)
                {
                    // ScrollArea handles its own clipping
                    children()[1]->render(gc, cx, cy, cw, ch, force);
                }
            }

            m_dirty = false;
            m_child_dirty = false;
        }

    protected:
        void draw(GraphicsContext &gc) override
        {
            auto *tm = application()->theme_manager.get();
            Color border_color = tm->get_color("window_border");
            Color bg_color = tm->get_color("window_bg");

            // Table background (Square)
            gc.setColor(bg_color);
            gc.fillRect(m_x, m_y, m_width, m_height, 0);

            // Subtle border (Square)
            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, 0);
        }

        void calculate_internal_layout()
        {
            if (children().size() < 2)
                return;

            // Calculate total width based on columns
            int total_col_width = 0;
            for (const auto &col : m_columns)
            {
                total_col_width += col.width;
            }

            int table_width = (m_width_mode == TableViewWidthMode::Fill)
                                  ? m_width
                                  : std::max(m_width, total_col_width);

            // Update header container size
            Widget *header = children()[0].get();
            int header_h = m_header_visible ? 32 : 0;
            header->set_size(table_width, header_h);

            ScrollArea *scroll = static_cast<ScrollArea *>(children()[1].get());
            header->set_position(m_x - scroll->scroll_x(), m_y);

            // Update scroll area size and position
            scroll->set_position(m_x, m_y + header_h);
            scroll->set_size(m_width, m_height - header_h);

            // Update content container size (inside scroll area)
            if (!scroll->children().empty())
            {
                Widget *content = scroll->children()[0].get();
                int total_height = 0;
                if (!m_data.empty())
                {
                    total_height = (int)m_data.size() * m_row_height;
                }
                content->set_size(table_width, total_height);
            }

            // Update filler header width
            if (!header->children().empty())
            {
                Widget *filler = header->children().back().get();
                int filler_width = table_width - total_col_width;
                if (filler_width > 0)
                {
                    filler->set_visible(true);
                    filler->set_fixed_size(filler_width);
                }
                else
                {
                    filler->set_visible(false);
                }
            }
        }

        void sync_header_scroll()
        {
            if (children().size() < 2)
                return;
            Widget *header = children()[0].get();
            ScrollArea *scroll = static_cast<ScrollArea *>(children()[1].get());
            header->set_position(m_x - scroll->scroll_x(), m_y);
            header->invalidate();
        }

        void rebuild_header()
        {
            if (children().empty())
                return;
            Widget *header = children()[0].get();
            header->clear_children();

            for (size_t i = 0; i < m_columns.size(); ++i)
            {
                auto btn = std::make_unique<Button<AquaObject>>();
                btn->set_text(m_columns[i].title);
                btn->set_corner_radius({0, 0, 0, 0});
                btn->set_fixed_size(m_columns[i].width);
                btn->set_font_weight(FONT_WEIGHT_BOLD);

                if (m_sort_column == (int)i)
                {
                    btn->set_accent_color(WidgetAccentColor::Primary);
                }

                if (m_columns[i].sortable)
                {
                    btn->when_mouse_press.connect([this, i](EventContext &) { sort_by_column(i); });
                }

                header->add_child(std::move(btn));
            }

            // Cosmetic filler header
            auto filler = std::make_unique<Button<AquaObject>>();
            filler->set_text("");
            filler->set_corner_radius({0, 0, 0, 0});
            filler->set_enabled(false); // No interaction
            header->add_child(std::move(filler));
        }

        void rebuild_content()
        {
            if (children().size() < 2)
                return;
            ScrollArea *scroll = static_cast<ScrollArea *>(children()[1].get());
            if (scroll->children().empty())
                return;

            Widget *content = scroll->children()[0].get();
            content->clear_children();

            for (size_t row_idx = 0; row_idx < m_data.size(); ++row_idx)
            {
                auto row_widget = std::make_unique<TableRow>();
                row_widget->set_fixed_size(m_row_height);

                if (m_use_alternate_colors)
                {
                    row_widget->set_alternate(row_idx % 2 != 0);
                }

                if (m_selected_row == (int)row_idx)
                {
                    row_widget->set_selected(true);
                }

                row_widget->when_mouse_press.connect(
                    [this, row_idx](EventContext &ctx)
                    {
                        if (ctx.button == 0x110) // Left click
                        {
                            m_selected_row = (int)row_idx;
                            rebuild_content();
                            if (m_on_row_selected)
                                m_on_row_selected(m_data[row_idx]);
                        }
                    });

                for (size_t col_idx = 0; col_idx < m_columns.size(); ++col_idx)
                {
                    std::unique_ptr<Widget> cell;
                    if (m_columns[col_idx].cell_factory)
                    {
                        cell = m_columns[col_idx].cell_factory(m_data[row_idx]);
                    }
                    else
                    {
                        auto lbl = std::make_unique<Label>("Cell");
                        cell = std::move(lbl);
                    }
                    cell->set_fixed_size(m_columns[col_idx].width);
                    row_widget->add_child(std::move(cell));
                }

                content->add_child(std::move(row_widget));
            }
            invalidate();
        }

        virtual void sort_by_column(size_t col_idx)
        {
            if (col_idx >= m_columns.size() || !m_columns[col_idx].sortable)
                return;

            if (m_columns[col_idx].sort_predicate)
            {
                if (m_sort_column == (int)col_idx)
                {
                    m_sort_ascending = !m_sort_ascending;
                }
                else
                {
                    m_sort_column = (int)col_idx;
                    m_sort_ascending = true;
                }

                std::sort(m_data.begin(), m_data.end(),
                          [this, col_idx](const T &a, const T &b)
                          {
                              if (m_sort_ascending)
                                  return m_columns[col_idx].sort_predicate(a, b);
                              else
                                  return m_columns[col_idx].sort_predicate(b, a);
                          });

                rebuild_header();
                rebuild_content();
            }
        }

    protected:
        std::vector<TableColumn<T>> m_columns;
        std::vector<T> m_data;

        std::unique_ptr<Widget> m_header_container;
        std::unique_ptr<ScrollArea> m_scroll_area;
        std::unique_ptr<Widget> m_content_container;

        TableViewWidthMode m_width_mode{TableViewWidthMode::Fill};
        bool m_header_visible{true};
        int m_row_height{28};

        int m_sort_column{-1};
        bool m_sort_ascending{true};

        bool m_use_alternate_colors{true};

        int m_selected_row{-1};
        std::function<void(const T &)> m_on_row_selected;
    };
} // namespace horizon
