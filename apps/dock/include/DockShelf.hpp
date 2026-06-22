#pragma once

#include <horizon/Widget.hpp>

namespace horizon
{

    /**
     * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
     */
    class DockItem;
    class Menu;
    class DockApplication;

    /**
     * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
     */
    class DockShelf : public Widget
    {
    public:
        DockShelf();

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

        void set_magnification_enabled(bool enabled) { m_magnification_enabled = enabled; }
        bool is_magnification_enabled() const { return m_magnification_enabled; }
        bool is_mouse_over() const { return m_mouse_over; }
        void set_base_size(int size) { m_base_size = size; m_max_extra_size = size; calculate_layout(); invalidate(); }
        void reset_magnification() { m_mouse_over = false; calculate_layout(); invalidate(); }
        int base_size() const { return m_base_size; }
        void set_dock_app(DockApplication* app) { m_dock_app = app; }
        void set_dock_position(const std::string& pos) { m_position = pos; invalidate(); }
        const std::string& dock_position() const { return m_position; }

    private:
        bool m_magnification_enabled = true;
        int m_base_size = 64;
        int m_max_extra_size = 64;
        DockApplication* m_dock_app = nullptr;
        int m_mouse_x = -1;
        int m_mouse_y = -1;
        bool m_mouse_over = false;

        DockItem* m_dragged_item = nullptr;
        int m_drag_mouse_x = -1;
        int m_drag_mouse_y = -1;
        int m_drag_target_index = -1;
        int m_drag_start_index = -1;

        std::string m_position = "bottom";

    public:
        void start_drag(DockItem* item, int mouse_x, int mouse_y);
        void update_drag(int mouse_x, int mouse_y);
        void end_drag();
        void cancel_drag();
    };

} // namespace horizon
