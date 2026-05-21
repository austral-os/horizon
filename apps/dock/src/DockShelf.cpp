#include "DockShelf.hpp"
#include "DockApplication.hpp"
#include "DockItem.hpp"
#include "horizon/Logger.hpp"
#include <cmath>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Icon.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/ThemeManager.hpp>
#include <vector>

namespace horizon
{

    DockShelf::DockShelf()
    {
        // Horizontal layout with padding for the shelf appearance
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        // Leave space at the bottom for the 3D lip of the shelf
        set_margin(20); // Changed from 10 to 20
        set_size(0, 100);

        when_mouse_move.connect(
            [this](MouseMoveEventContext &ctx)
            {
                if (m_magnification_enabled)
                {
                    widget_position pos = get_absolute_position();
                    int sx = application() ? application()->screen_x() : 0;
                    int sy = application() ? application()->screen_y() : 0;
                    int shelf_x = pos.x - sx;
                    int shelf_y = pos.y - sy;
                    m_mouse_x = (int)ctx.x - shelf_x;
                    m_mouse_y = (int)ctx.y - shelf_y;
                    m_mouse_over = true;
                    invalidate();
                    calculate_layout();
                }
            });

        when_mouse_leave.connect(
            [this](EventContext &ctx)
            {
                if (m_magnification_enabled)
                {
                    m_mouse_over = false;
                    invalidate();
                    calculate_layout();
                }
            });
    }

    void DockShelf::calculate_layout()
    {
        const float radius = 120.0f;

        int total_children_length = 0;
        int count = 0;
        bool is_vertical = (m_position == "left" || m_position == "right");

        for (const auto &child : children())
        {
            if (child->is_visible())
            {
                if (m_dragged_item == child.get())
                {
                    count++;
                    continue; // Skip dragged item in normal layout
                }

                // If this is the target index for the dragged item, leave a gap
                if (count == m_drag_target_index)
                {
                    total_children_length += m_base_size + spacing();
                }

                Icon *icon_child = dynamic_cast<Icon *>(child.get());
                int current_size = child->fixed_size();
                float current_width = current_size;
                float current_height = current_size;
                float length_to_add = current_size;

                if (icon_child)
                {
                    int current_icon_size = m_base_size;
                    if (m_magnification_enabled && m_mouse_over && !m_dragged_item)
                    {
                        float child_center = (float)total_children_length + child->fixed_size() / 2.0f;
                        float dist_primary = std::abs((child_center + margin()) - (is_vertical ? m_mouse_y : m_mouse_x));

                        float total_thickness = m_base_size * 2.5f;
                        float active_secondary;
                        if (m_position == "left") {
                            active_secondary = 19.0f + m_base_size / 2.0f;
                        } else if (m_position == "right") {
                            active_secondary = total_thickness - 19.0f - m_base_size / 2.0f;
                        } else {
                            float lip_height = 10.0f;
                            active_secondary = (total_thickness - lip_height - 9) - (m_base_size / 2.0f);
                        }
                        
                        float dist_secondary = std::abs(active_secondary - (is_vertical ? m_mouse_x : m_mouse_y));

                        float radius_secondary = 80.0f;
                        float scale_primary = std::exp(-(dist_primary * dist_primary) / (2 * radius * radius));
                        float scale_secondary = std::exp(-(dist_secondary * dist_secondary) / (2 * radius_secondary * radius_secondary));
                        float scale = scale_primary * scale_secondary;

                        current_icon_size = m_base_size + (int)(m_max_extra_size * scale);
                    }

                    child->set_fixed_size(current_icon_size + child->margin() * 2);
                    icon_child->set_icon_size(current_icon_size);
                    current_size = child->fixed_size();
                    current_width = current_size;
                    current_height = current_size;
                    length_to_add = current_size;
                }
                else
                {
                    if (is_vertical) {
                        current_width = m_base_size * 0.6f;
                        current_height = child->fixed_size(); // 2
                        length_to_add = current_height;
                    } else {
                        current_width = child->fixed_size(); // 2
                        current_height = m_base_size * 0.6f;
                        length_to_add = current_width;
                    }
                }

                if (count > 0)
                    total_children_length += spacing();

                float total_thickness = m_base_size * 2.5f;

                if (m_position == "left")
                {
                    float wall_offset = 19.0f; // 10px lip + 9px padding
                    if (icon_child) {
                        child->set_position(x() + wall_offset, y() + margin() + total_children_length);
                    } else {
                        float center_x = wall_offset + m_base_size / 2.0f;
                        child->set_position(x() + center_x - current_width / 2.0f, y() + margin() + total_children_length);
                    }
                    child->set_size(current_width, current_height);
                }
                else if (m_position == "right")
                {
                    float wall_offset = 19.0f;
                    int icon_right_x = total_thickness - wall_offset;
                    if (icon_child) {
                        child->set_position(x() + icon_right_x - current_width, y() + margin() + total_children_length);
                    } else {
                        float center_x = icon_right_x - m_base_size / 2.0f;
                        child->set_position(x() + center_x - current_width / 2.0f, y() + margin() + total_children_length);
                    }
                    child->set_size(current_width, current_height);
                }
                else
                {
                    float lip_height = 10.0f;
                    int icon_bottom_y = total_thickness - lip_height - 9;
                    if (icon_child) {
                        child->set_position(x() + margin() + total_children_length, y() + icon_bottom_y - current_height);
                    } else {
                        float center_y = icon_bottom_y - m_base_size / 2.0f;
                        child->set_position(x() + margin() + total_children_length, y() + center_y - current_height / 2.0f);
                    }
                    child->set_size(current_width, current_height);
                }
                
                child->calculate_layout();
                total_children_length += length_to_add;
                count++;
            }
        }

        if (m_dragged_item && m_drag_target_index >= count)
        {
            total_children_length += m_base_size + spacing();
        }

        if (m_dragged_item)
        {
            m_dragged_item->set_position(m_drag_mouse_x - m_dragged_item->width() / 2, m_drag_mouse_y - m_dragged_item->height() / 2);
        }

        int content_length = total_children_length + (margin() * 2);
        float total_thickness = m_base_size * 2.5f;
        
        if (is_vertical)
        {
            set_size(total_thickness, content_length);
            set_height(content_length);
            set_fixed_size(content_length);
        }
        else
        {
            set_size(content_length, total_thickness);
            set_width(content_length);
            set_fixed_size(content_length);
        }

        Widget::calculate_layout();
    }

    void DockShelf::draw(GraphicsContext &gc)
    {
        // Clear the widget area to prevent redrawing artifacts over old frames
        gc.clearRect(x(), y(), width(), height());
        auto& theme = ThemeManager::instance();

        if (m_position == "left" || m_position == "right")
        {
            float w = width();
            float h = height();
            float lip_size = 10.0f;
            float tray_depth = m_base_size * 1.5625f;
            float perspective_offset = 20.0f;

            Color surface_top = theme.get_color("dock_surface1").with_alpha(0.2f);
            Color surface_bottom = theme.get_color("dock_surface2").with_alpha(0.6f);
            Color lip_top = theme.get_color("dock_lip1").with_alpha(0.9f);
            Color lip_bottom = theme.get_color("dock_lip2").with_alpha(0.8f);
            Color glare = theme.get_color("dock_glare").with_alpha(0.8f);
            Color shadow = theme.get_color("dock_shadow").with_alpha(0.5f);
            Color separator = theme.get_color("dock_glare").with_alpha(0.3f);

            if (m_position == "left")
            {
                float lip_start_x = 0;
                float tray_front_x = lip_size;
                float tray_back_x = tray_depth;

                std::vector<PolygonPoint> surface_points = {
                    {static_cast<int>(x() + tray_front_x), static_cast<int>(y()), 0},
                    {static_cast<int>(x() + tray_back_x), static_cast<int>(y() + perspective_offset), 0},
                    {static_cast<int>(x() + tray_back_x), static_cast<int>(y() + h - perspective_offset), 0},
                    {static_cast<int>(x() + tray_front_x), static_cast<int>(y() + h), 0}
                };

                gc.fillLinearGradientPolygon(surface_points, surface_bottom, surface_top, false);
                gc.fillLinearGradientRect(x() + lip_start_x, y(), lip_size, h, lip_bottom, lip_top, false, CornerRadius(4, 0, 0, 4));

                gc.setColor(glare);
                gc.drawLine(x() + tray_front_x - 1, y(), x() + tray_front_x - 1, y() + h, 1.0f);

                gc.setColor(shadow);
                gc.drawLine(x() + lip_start_x + 1, y() + 4, x() + lip_start_x + 1, y() + h - 4, 1.0f);

                gc.setColor(separator);
                gc.drawLine(x() + tray_back_x - 1, y() + perspective_offset, x() + tray_back_x - 1, y() + h - perspective_offset, 1.0f);
            }
            else // right
            {
                float tray_front_x = w - lip_size;
                float tray_back_x = w - tray_depth;
                float lip_start_x = w - lip_size;

                std::vector<PolygonPoint> surface_points = {
                    {static_cast<int>(x() + tray_back_x), static_cast<int>(y() + perspective_offset), 0},
                    {static_cast<int>(x() + tray_front_x), static_cast<int>(y()), 0},
                    {static_cast<int>(x() + tray_front_x), static_cast<int>(y() + h), 0},
                    {static_cast<int>(x() + tray_back_x), static_cast<int>(y() + h - perspective_offset), 0}
                };

                gc.fillLinearGradientPolygon(surface_points, surface_top, surface_bottom, false);
                gc.fillLinearGradientRect(x() + lip_start_x, y(), lip_size, h, lip_top, lip_bottom, false, CornerRadius(0, 4, 4, 0));

                gc.setColor(glare);
                gc.drawLine(x() + tray_front_x + 1, y(), x() + tray_front_x + 1, y() + h, 1.0f);

                gc.setColor(shadow);
                gc.drawLine(x() + w - 1, y() + 4, x() + w - 1, y() + h - 4, 1.0f);

                gc.setColor(separator);
                gc.drawLine(x() + tray_back_x, y() + perspective_offset, x() + tray_back_x, y() + h - perspective_offset, 1.0f);
            }
            
            gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
            return;
        }

        // Original horizontal 3D drawing for "bottom"
        float w = width();
        float h = height(); // 160

        // Shelf geometry measurements
        float lip_height = 10.0f;
        float tray_height = m_base_size * 1.5625f;
        float tray_top_y = h - tray_height;
        float tray_bottom_y = h - lip_height;

        float perspective_offset = 20.0f; // Trapezoid slant width

        // 1. Draw the tray surface (Trapezoid from background to foreground)
        std::vector<PolygonPoint> surface_points = {
            {static_cast<int>(x() + perspective_offset), static_cast<int>(y() + tray_top_y),
             0}, // Top Left
            {static_cast<int>(x() + w - perspective_offset), static_cast<int>(y() + tray_top_y),
             0},                                                                   // Top Right
            {static_cast<int>(x() + w), static_cast<int>(y() + tray_bottom_y), 0}, // Bottom Right
            {static_cast<int>(x()), static_cast<int>(y() + tray_bottom_y), 0}      // Bottom Left
        };
        
        Color surface_top = theme.get_color("dock_surface1").with_alpha(0.2f);
        Color surface_bottom = theme.get_color("dock_surface2").with_alpha(0.6f);

        // Soft translucent gradient for the glass shelf
        gc.fillLinearGradientPolygon(surface_points, surface_top, surface_bottom, true);

        // 2. Draw the front lip (Edge)
        Color lip_top = theme.get_color("dock_lip1").with_alpha(0.9f);
        Color lip_bottom = theme.get_color("dock_lip2").with_alpha(0.8f);
        
        gc.fillLinearGradientRect(x(), y() + tray_bottom_y, w, lip_height,
                                  lip_top, lip_bottom, true, CornerRadius(0, 0, 4, 4));

        // 3. Glare/Shine effects
        // Top edge of the lip shine
        Color glare = theme.get_color("dock_glare").with_alpha(0.8f);
        gc.setColor(glare);
        gc.drawLine(x(), y() + tray_bottom_y + 1, x() + w, y() + tray_bottom_y + 1, 1.0f);

        // Bottom edge of the lip shadow
        Color shadow = theme.get_color("dock_shadow").with_alpha(0.5f);
        gc.setColor(shadow);
        gc.drawLine(x() + 4, y() + tray_bottom_y + lip_height, x() + w - 4,
                    y() + tray_bottom_y + lip_height, 1.0f);

        // Separating line acting as the back wall intersection
        Color separator = theme.get_color("dock_glare").with_alpha(0.3f);
        gc.setColor(separator);
        gc.drawLine(x() + perspective_offset, y() + tray_top_y, x() + w - perspective_offset,
                    y() + tray_top_y, 1.0f);

        gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    void DockShelf::start_drag(DockItem *item, int mouse_x, int mouse_y)
    {
        m_dragged_item = item;
        m_drag_mouse_x = mouse_x;
        m_drag_mouse_y = mouse_y;

        // Find current index
        m_drag_start_index = -1;
        int i = 0;
        for (const auto &child : children())
        {
            if (child.get() == item)
            {
                m_drag_start_index = i;
                break;
            }
            i++;
        }
        m_drag_target_index = m_drag_start_index;
        calculate_layout();
        invalidate();
    }

    void DockShelf::update_drag(int mouse_x, int mouse_y)
    {
        m_drag_mouse_x = mouse_x;
        m_drag_mouse_y = mouse_y;

        // Calculate target index
        int new_target_index = 0;
        int current_x = x() + margin();
        for (const auto &child : children())
        {
            if (child.get() == m_dragged_item)
                continue;
            if (m_drag_mouse_x < current_x + child->width() / 2)
            {
                break;
            }
            current_x += child->width() + spacing();
            new_target_index++;
        }

        if (new_target_index != m_drag_target_index)
        {
            m_drag_target_index = new_target_index;
            calculate_layout();
            invalidate();
        }
        else
        {
            // Even if index didn't change, the dragged icon position did
            calculate_layout();
            invalidate();
        }
    }

    void DockShelf::end_drag()
    {
        if (!m_dragged_item)
            return;

        DockApplication *app = m_dock_app;
        if (app)
        {
            // 1. Check if dropped outside
            // Get absolute mouse position
            // Since mouse_x/y are relative to the window, we check if they are outside the window
            // area Or better, check if they are significantly away from the dock shelf

            bool outside = false;
            // The dock is at the bottom. If mouse is too high, it's outside.
            if (m_drag_mouse_y < y() - 50 || m_drag_mouse_y > y() + height() + 50 ||
                m_drag_mouse_x < x() - 50 || m_drag_mouse_x > x() + width() + 50)
            {
                outside = true;
            }

            if (outside)
            {
                if (m_dragged_item->is_pinned())
                {
                    app->unpin_app(m_dragged_item->app_id());
                }
            }
            else
            {
                // 2. Inside: Reorder or Pin
                if (m_dragged_item->is_pinned())
                {
                    // Reorder pinned apps
                    // Note: m_drag_target_index is in terms of children(), but we need to map it to
                    // pinned_apps index Actually, reorder_pinned_app works on pinned_apps indices.
                    // We assume pinned apps are the first N children.

                    int old_pinned_index = m_drag_start_index;
                    int new_pinned_index = m_drag_target_index;

                    LOG_INFO << "[DRAG] Reordering pinned app from index " << old_pinned_index
                             << " to index " << new_pinned_index << std::endl;

                    app->reorder_pinned_app(old_pinned_index, new_pinned_index);
                }
                else
                {
                    LOG_INFO << "[DRAG] Reordering non-pinned app from index "
                             << m_drag_target_index << std::endl;
                    // Pin unpinned app at target position
                    app->pin_app_at(m_dragged_item->app_id(), m_dragged_item->name(),
                                    m_dragged_item->icon_name(), m_dragged_item->run_id(),
                                    m_drag_target_index);
                }
            }
        }

        m_dragged_item = nullptr;
        m_drag_target_index = -1;
        m_drag_start_index = -1;
        calculate_layout();
        invalidate();
    }

    void DockShelf::cancel_drag()
    {
        m_dragged_item = nullptr;
        m_drag_target_index = -1;
        m_drag_start_index = -1;
        calculate_layout();
        invalidate();
    }
} // namespace horizon
