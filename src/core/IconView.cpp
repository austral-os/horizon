#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconView.hpp>

namespace horizon
{
    IconViewBase::IconViewBase() : Widget()
    {
        // Theme color will be applied when added to an application


        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(FREE);
        m_scroll_area = scroll_area.get();

        auto content_pane = std::make_unique<Widget>();
        content_pane->set_position_type(FREE);
        m_content_pane = content_pane.get();

        m_scroll_area->set_content(std::move(content_pane));
        add_child(std::move(scroll_area));

        m_scroll_area->when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
            if (ctx.button == 0x110 || ctx.button == 0x111)
            {
                if (m_rubberband_selection_enabled && ctx.button == 0x110) {
                    m_is_rubberbanding = true;
                    m_rubberband_start_x = ctx.x - m_x + m_scroll_area->scroll_x();
                    m_rubberband_start_y = ctx.y - m_y + m_scroll_area->scroll_y();
                    m_rubberband_current_x = m_rubberband_start_x;
                    m_rubberband_current_y = m_rubberband_start_y;

                    bool ctrl_pressed = (ctx.modifiers & WaylandWindow::Modifier::CTRL);
                    bool shift_pressed = (ctx.modifiers & WaylandWindow::Modifier::SHIFT);

                    if (!ctrl_pressed && !shift_pressed) {
                        clear_selection();
                        m_initial_selection.clear();
                    } else {
                        m_initial_selection = m_selected_indices;
                    }
                } else {
                    clear_selection();
                }
            }
        });

        m_scroll_area->when_mouse_drag.connect([this](MouseMoveEventContext &ctx) {
            if (m_is_rubberbanding) {
                m_rubberband_current_x = ctx.x - m_x + m_scroll_area->scroll_x();
                m_rubberband_current_y = ctx.y - m_y + m_scroll_area->scroll_y();

                int rx = std::min(m_rubberband_start_x, m_rubberband_current_x);
                int ry = std::min(m_rubberband_start_y, m_rubberband_current_y);
                int rw = std::abs(m_rubberband_start_x - m_rubberband_current_x);
                int rh = std::abs(m_rubberband_start_y - m_rubberband_current_y);

                m_selected_indices = m_initial_selection;

                auto &children = m_content_pane->children();
                for (int i = 0; i < (int)children.size(); ++i) {
                    auto &child = children[i];
                    int cx = child->x() - m_x + m_scroll_area->scroll_x();
                    int cy = child->y() - m_y + m_scroll_area->scroll_y();
                    int cw = child->width();
                    int ch = child->height();

                    if (rx < cx + cw && rx + rw > cx &&
                        ry < cy + ch && ry + rh > cy) {
                        m_selected_indices.insert(i);
                    }
                }

                if (!m_autoscroll_timer && application()) {
                    bool near_edge = false;
                    int scroll_y = m_scroll_area->scroll_y();
                    int local_y = ctx.y - m_y;
                    if (local_y < 20 && scroll_y > 0) near_edge = true;
                    if (local_y > m_scroll_area->height() - 20) near_edge = true;

                    if (near_edge) {
                        m_autoscroll_timer = application()->add_timer(30, [this]() {
                            if (!m_is_rubberbanding) return;
                            int scroll_y = m_scroll_area->scroll_y();
                            
                            // Use window coordinates to calculate local_y
                            // But wait, the timer doesn't have ctx.y.
                            // We can use m_rubberband_current_y which is content coordinate, 
                            // to infer local_y.
                            int local_y = m_rubberband_current_y - m_scroll_area->scroll_y();
                            
                            if (local_y < 20 && scroll_y > 0) {
                                m_scroll_area->set_scroll_position(m_scroll_area->scroll_x(), std::max(0, scroll_y - 10));
                            } else if (local_y > m_scroll_area->height() - 20) {
                                m_scroll_area->set_scroll_position(m_scroll_area->scroll_x(), scroll_y + 10);
                            }
                            
                            int rx = std::min(m_rubberband_start_x, m_rubberband_current_x);
                            int ry = std::min(m_rubberband_start_y, m_rubberband_current_y);
                            int rw = std::abs(m_rubberband_start_x - m_rubberband_current_x);
                            int rh = std::abs(m_rubberband_start_y - m_rubberband_current_y);
                            
                            m_selected_indices = m_initial_selection;
                            auto &children = m_content_pane->children();
                            for (int i = 0; i < (int)children.size(); ++i) {
                                auto &child = children[i];
                                int cx = child->x() - m_x + m_scroll_area->scroll_x();
                                int cy = child->y() - m_y + m_scroll_area->scroll_y();
                                int cw = child->width();
                                int ch = child->height();

                                if (rx < cx + cw && rx + rw > cx &&
                                    ry < cy + ch && ry + rh > cy) {
                                    m_selected_indices.insert(i);
                                }
                            }
                            invalidate();
                            
                            local_y = m_rubberband_current_y - m_scroll_area->scroll_y();
                            if (local_y >= 20 && local_y <= m_scroll_area->height() - 20) {
                                if (m_autoscroll_timer && application()) {
                                    application()->stop_timer(m_autoscroll_timer);
                                    m_autoscroll_timer = 0;
                                }
                            }
                        }, true);
                    }
                }
                
                int local_y = ctx.y - m_y;
                if (m_autoscroll_timer && application() && local_y >= 20 && local_y <= m_scroll_area->height() - 20) {
                    application()->stop_timer(m_autoscroll_timer);
                    m_autoscroll_timer = 0;
                }

                invalidate();
            }
        });

        m_scroll_area->when_mouse_release.connect([this](MouseButtonEventContext &ctx) {
            if (m_is_rubberbanding && ctx.button == 0x110) {
                m_is_rubberbanding = false;
                if (m_autoscroll_timer && application()) {
                    application()->stop_timer(m_autoscroll_timer);
                    m_autoscroll_timer = 0;
                }
                rebuild_items();
                invalidate();
            }
        });
    }

    void IconViewBase::set_zoom(float zoom)
    {
        float clamped_zoom = std::max(0.2f, std::min(zoom, 5.0f));
        if (m_zoom != clamped_zoom)
        {
            m_zoom = clamped_zoom;
            rebuild_items();
        }
    }

    float IconViewBase::zoom() const
    {
        return m_zoom;
    }

    void IconViewBase::set_rubberband_selection_enabled(bool enabled)
    {
        m_rubberband_selection_enabled = enabled;
    }

    bool IconViewBase::rubberband_selection_enabled() const
    {
        return m_rubberband_selection_enabled;
    }

    void IconViewBase::draw(GraphicsContext &gc)
    {
        Widget::draw(gc);

        if (m_is_rubberbanding)
        {
            int rx = std::min(m_rubberband_start_x, m_rubberband_current_x) + m_x - m_scroll_area->scroll_x();
            int ry = std::min(m_rubberband_start_y, m_rubberband_current_y) + m_y - m_scroll_area->scroll_y();
            int rw = std::abs(m_rubberband_start_x - m_rubberband_current_x);
            int rh = std::abs(m_rubberband_start_y - m_rubberband_current_y);

            Color theme_color = Color(0.2f, 0.5f, 0.9f, 0.3f);
            Color stroke_color = Color(0.2f, 0.5f, 0.9f, 0.8f);
            if (theme_manager()) {
                Color base = theme_manager()->get_color("table_row_selected");
                theme_color = Color(base.r, base.g, base.b, 0.3f);
                stroke_color = Color(base.r, base.g, base.b, 0.8f);
            }

            gc.save();
            gc.clip(m_x, m_y, m_width, m_height);

            gc.setColor(theme_color);
            gc.fillRect(rx, ry, rw, rh);

            gc.setColor(stroke_color);
            gc.drawRect(rx, ry, rw, rh, 0, 1.0f);

            gc.restore();
        }
    }

    void IconViewBase::set_transparent(bool transparent)
    {
        m_transparent = transparent;
        if (transparent)
        {
            set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
        }
        else if (theme_manager())
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
        }
        else
        {
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f)); // fallback
        }
    }

    void IconViewBase::set_layout_mode(IconViewLayoutMode mode)
    {
        if (m_layout_mode != mode)
        {
            m_layout_mode = mode;
            invalidate();
            calculate_layout();
        }
    }

    IconViewLayoutMode IconViewBase::layout_mode() const
    {
        return m_layout_mode;
    }

    void IconViewBase::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        if (theme_manager() && !m_transparent)
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
        }
    }

    void IconViewBase::set_selected_index(int index, bool ctrl, bool shift)
    {
        if (shift && m_selected_index != -1)
        {
            int start = std::min(m_selected_index, index);
            int end = std::max(m_selected_index, index);
            
            if (!ctrl)
            {
                m_selected_indices.clear();
            }
            
            for (int i = start; i <= end; ++i)
            {
                m_selected_indices.insert(i);
            }
            // In shift selection, the anchor (m_selected_index) remains the same.
        }
        else if (ctrl)
        {
            // Toggle selection
            if (m_selected_indices.count(index))
                m_selected_indices.erase(index);
            else
                m_selected_indices.insert(index);

            // Update anchor to the last-toggled index
            m_selected_index = index;
        }
        else
        {
            if (m_selected_indices.size() == 1 && m_selected_indices.count(index))
                return; // Already exclusively selected, nothing to do

            m_selected_indices.clear();
            if (index >= 0)
                m_selected_indices.insert(index);
            m_selected_index = index;
        }

        // Defer rebuild to avoid destroying the widget tree during event handling
        if (application())
        {
            application()->add_timer(0, [this]() { rebuild_items(); });
        }
        else
        {
            rebuild_items();
        }
    }

    void IconViewBase::clear_selection()
    {
        if (m_selected_indices.empty())
            return;
        m_selected_indices.clear();
        m_selected_index = -1;
        if (application())
            application()->add_timer(0, [this]() { rebuild_items(); });
        else
            rebuild_items();
    }

    int IconViewBase::selected_index() const
    {
        return m_selected_index;
    }
    void IconViewBase::set_side_margin(int margin)
    {
        if (m_side_margin != margin)
        {
            m_side_margin = margin;
            invalidate();
            calculate_layout();
        }
    }

    int IconViewBase::side_margin() const
    {
        return m_side_margin;
    }

    void IconViewBase::set_item_size(int width, int height)
    {
        BASE_ITEM_WIDTH = width;
        BASE_ITEM_HEIGHT = height;
        invalidate();
        calculate_layout();
    }

    int IconViewBase::get_theme_font_size(const std::string &role) const
    {
        if (!theme_manager())
            return 12;

        auto fd = theme_manager()->get_font(role);
        if (fd.size > 0)
            return fd.size;

        // Fallback to window role
        fd = theme_manager()->get_font("window");
        if (fd.size > 0)
            return fd.size;

        return 12; // Hard fallback
    }

    void IconViewBase::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_width <= 0 || m_height <= 0)
        {
            if (m_parent && m_parent->width() > 0 && m_parent->height() > 0)
            {
                m_width = m_parent->width();
                m_height = m_parent->height();
            }
            else
            {
                return;
            }
        }

        if (m_scroll_area)
        {
            m_scroll_area->set_position(m_x, m_y);
            m_scroll_area->set_size(m_width, m_height);
        }

        m_item_width = std::max(16, static_cast<int>(BASE_ITEM_WIDTH * m_zoom));
        m_item_height = std::max(16, static_cast<int>(BASE_ITEM_HEIGHT * m_zoom));
        m_grid_spacing = std::max(0, static_cast<int>(BASE_GRID_SPACING * m_zoom));

        int side_margin = std::max(0, static_cast<int>(m_side_margin * m_zoom));
        int available_width = m_width - 2 * side_margin;

        if (available_width <= 0)
            available_width = m_width;

        m_rows_count = std::max(1, (m_height - 2 * side_margin) / (m_item_height + m_grid_spacing));

        int scroll_x = m_scroll_area ? m_scroll_area->scroll_x() : 0;
        int scroll_y = m_scroll_area ? m_scroll_area->scroll_y() : 0;
        
        auto &children = m_content_pane->children();

        if (m_layout_mode == IconViewLayoutMode::Horizontal) {
            int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));
            if (columns > 1 &&
                (columns * (m_item_width + m_grid_spacing) - m_grid_spacing) > available_width)
            {
                columns--;
                if (columns < 1)
                    columns = 1;
            }
            
            m_columns_count = columns;

            // Center the grid horizontally
            int grid_width = columns * (m_item_width + m_grid_spacing) - m_grid_spacing;
            int start_x = side_margin + (available_width - grid_width) / 2;

            int actual_spacing = m_grid_spacing;
            if (columns > 1)
            {
                actual_spacing = (available_width - columns * m_item_width) / (columns - 1);
                start_x = side_margin;
            }

            int current_y = side_margin;

            for (int i = 0; i < (int)children.size(); i += columns)
            {
                int row_max_height = 0;
                int row_end = std::min(i + columns, (int)children.size());

                for (int j = i; j < row_end; ++j)
                {
                    row_max_height =
                        std::max(row_max_height, children[j]->preferred_height(m_item_width));
                }

                for (int j = i; j < row_end; ++j)
                {
                    int col = j - i;
                    int x = m_x + start_x + col * (m_item_width + actual_spacing) - scroll_x;
                    int y = m_y + current_y - scroll_y;
                    children[j]->set_position(x, y);
                    children[j]->set_size(m_item_width, row_max_height);
                }

                current_y += row_max_height + m_grid_spacing;
            }

            int needed_height = current_y + side_margin - (children.empty() ? 0 : m_grid_spacing);
            needed_height = std::min(1000000, std::max(m_height, needed_height));
            m_content_pane->set_size(m_width, needed_height);
            
        } else {
            // Vertical Modes
            int available_height = m_height - 2 * side_margin;
            if (available_height <= 0) available_height = m_height;

            int current_y = side_margin;
            int current_col = 0;
            int max_y_reached = side_margin;
            
            for (int i = 0; i < (int)children.size(); ++i) {
                int item_h = children[i]->preferred_height(m_item_width);
                
                // Check if we need to wrap to the next column
                if (current_y + item_h > m_height - side_margin && current_y > side_margin) {
                    current_col++;
                    current_y = side_margin;
                }
                
                int x = 0;
                if (m_layout_mode == IconViewLayoutMode::VerticalRightToLeft) {
                    x = m_x + m_width - side_margin - m_item_width - current_col * (m_item_width + m_grid_spacing) - scroll_x;
                } else {
                    x = m_x + side_margin + current_col * (m_item_width + m_grid_spacing) - scroll_x;
                }
                
                int y = m_y + current_y - scroll_y;
                
                children[i]->set_position(x, y);
                children[i]->set_size(m_item_width, item_h);
                
                current_y += item_h + m_grid_spacing;
                if (current_y > max_y_reached) max_y_reached = current_y;
            }
            
            int needed_width = side_margin + (current_col + 1) * (m_item_width + m_grid_spacing);
            int needed_height = std::max(m_height, max_y_reached + side_margin - m_grid_spacing);
            
            m_columns_count = current_col + 1; // Update for generic calculations
            
            m_content_pane->set_size(std::max(m_width, needed_width), needed_height);
        }
    }

} // namespace horizon
